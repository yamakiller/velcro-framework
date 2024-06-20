#ifndef V_FRAMEWORK_VTEST_VTEST_H
#define V_FRAMEWORK_VTEST_VTEST_H

#include <vcore/platform_default.h>
#include <vtest_traits_platform.h>
#include <list>
#include <array>

V_PUSH_DISABLE_WARNING(4389 4800, "-Wunknown-warning-option"); // 'int' : forcing value to bool 'true' or 'false' (performance warning).
#undef strdup // This define is required by googletest
#include <gtest/gtest.h>
#include <gmock/gmock.h>
V_POP_DISABLE_WARNING;

#if defined(HAVE_BENCHMARK)
#include <benchmark/benchmark.h>
#endif

#include <vcore/memory/osallocator.h>

#define VTEST_DLL_PUBLIC V_DLL_EXPORT
#define VTEST_EXPORT extern "C" VTEST_DLL_PUBLIC

namespace V {
    namespace Test {
        class Platform;

        /// @brief 实现此接口来定义环境设置和拆卸功能
        class ITestEnvironment : public ::testing::Environment {
        public:
            virtual ~ITestEnvironment()
            {}

            void SetUp() final
            {
                SetupEnvironment();
            }

            void TearDown() final
            {
                TeardownEnvironment();
            }

        protected:
            virtual void SetupEnvironment() = 0;
            virtual void TeardownEnvironment() = 0;
        };

        extern ::testing::Environment* sTestEnvironment;

        /// @brief 单体构建将拥有所有可用的环境. 保留映射以运行所需的环境;
        class TestEnvironmentRegistry {
        public:
            TestEnvironmentRegistry(std::vector<ITestEnvironment*> envs, const std::string& module_name, bool unit)
                : ModuleName(module_name)
                , Envs(envs)
                , Unit(unit)
            {
                _envs.push_back(this);
            }

            const std::string ModuleName;
            std::vector<ITestEnvironment*> Envs;
            bool Unit;

            static std::vector<TestEnvironmentRegistry*> _envs;

        private:
            TestEnvironmentRegistry& operator=(const TestEnvironmentRegistry& tmp);
        };

        /// @brief ITestEnvironment 的空实现
        class EmptyTestEnvironment final : public ITestEnvironment {
        public:
            virtual ~EmptyTestEnvironment()
            {}

        protected:
            void SetupEnvironment() override
            {}
            void TeardownEnvironment() override
            {}
        };

        void addTestEnvironment(ITestEnvironment* env);
        void addTestEnvironments(std::vector<ITestEnvironment*> envs);
        void excludeIntegTests();
        
        //! A hook that can be used to read any other misc parameters and remove them before google sees them.
        //! Note that this modifies argc and argv to delete the parameters it consumes.
        void ApplyGlobalParameters(int* argc, char** argv);
        void printUnusedParametersWarning(int argc, char** argv);

        class VelcroUnitTestMain final
        {
        public:
            VelcroUnitTestMain(std::vector<V::Test::ITestEnvironment*> envs)
                : m_returnCode(0)
                , m_envs(envs)
            {}

            bool Run(int argc, char** argv);
            bool Run(const char* commandLine);
            int ReturnCode() const { return m_returnCode; }

        private:
            int m_returnCode;
            std::vector<ITestEnvironment*> m_envs;
        };

        //! Run tests in a single library by loading it dynamically and executing the exported symbol,
        //! passing main-like parameters (argc, argv) from the (real or artificial) command line.
        int RunTestsInLib(Platform& platform, const std::string& lib, const std::string& symbol, int& argc, char** argv);
#if defined(HAVE_BENCHMARK)
         static constexpr const char* _benchmarkEnvironmentName = "BenchmarkEnvironment";

        // BenchmarkEnvironment is a base that can be implemented to used to perform global initialization and teardown
        // for a module
        class BenchmarkEnvironmentBase
        {
        public:
            virtual ~BenchmarkEnvironmentBase() = default;

            virtual void SetUpBenchmark()
            {
            }
            virtual void TearDownBenchmark()
            {
            }
        };

        class BenchmarkEnvironmentRegistry
        {
        public:
            BenchmarkEnvironmentRegistry() = default;
            BenchmarkEnvironmentRegistry(const BenchmarkEnvironmentRegistry&) = delete;
            BenchmarkEnvironmentRegistry& operator=(const BenchmarkEnvironmentRegistry&) = delete;

            void AddBenchmarkEnvironment(std::unique_ptr<BenchmarkEnvironmentBase> env)
            {
                m_envs.push_back(std::move(env));
            }

            std::vector<std::unique_ptr<BenchmarkEnvironmentBase>>& GetBenchmarkEnvironments()
            {
                return m_envs;
            }

        private:
            std::vector<std::unique_ptr<BenchmarkEnvironmentBase>> m_envs;
        };

        /*
         * Creates a BenchmarkEnvironment using the specified template type and registers it with the BenchmarkEnvironmentRegister
         * @param T template argument that must have BenchmarkEnvironmentBase as a base class
         * @return returns a reference to the created BenchmarkEnvironment
         */
        template<typename T>
        T& RegisterBenchmarkEnvironment()
        {
            static_assert(std::is_base_of<BenchmarkEnvironmentBase, T>::value, "Supplied benchmark environment must be derived from BenchmarkEnvironmentBase");

            static V::EnvironmentVariable<V::Test::BenchmarkEnvironmentRegistry> _benchmarkRegistry;
            if (!_benchmarkRegistry)
            {
                _benchmarkRegistry = V::Environment::CreateVariable<V::Test::BenchmarkEnvironmentRegistry>(_benchmarkEnvironmentName);
            }

            auto benchmarkEnv{ new T };
            _benchmarkRegistry->AddBenchmarkEnvironment(std::unique_ptr<BenchmarkEnvironmentBase>{ benchmarkEnv });
            return *benchmarkEnv;
        }

        template<typename... Ts>
        std::array<BenchmarkEnvironmentBase*, sizeof...(Ts)> RegisterBenchmarkEnvironments()
        {
            constexpr size_t EnvironmentCount{ sizeof...(Ts) };
            if constexpr (EnvironmentCount)
            {
                std::array<BenchmarkEnvironmentBase*, EnvironmentCount> benchmarkEnvs{ { &RegisterBenchmarkEnvironment<Ts>()... } };
                return benchmarkEnvs;
            }
            else
            {
                std::array<BenchmarkEnvironmentBase*, EnvironmentCount> benchmarkEnvs{};
                return benchmarkEnvs;
            }
        }
#endif // HAVE_BENCHMARK
         ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //! listener class to capture and print test output for embedded platforms
        class OutputEventListener : public ::testing::EmptyTestEventListener  {
        public:
            std::list<std::string> resultList;
            
            void OnTestEnd(const ::testing::TestInfo& test_info) override {
                std::string result;
                if (test_info.result()->Failed()) {
                    result = "Fail";
                } else {
                    result = "Pass";
                }
                std::string formattedResult = "[GTEST][" + result + "] " + test_info.test_case_name() + " " + test_info.name() + "\n";
                resultList.emplace_back(formattedResult);
            }

            void OnTestProgramEnd(const ::testing::UnitTest& unit_test) override
            {
                for (std::string testResults : resultList) {
                    V_Printf("", testResults.c_str());
                }
                if (unit_test.current_test_info()) {
                    V_Printf("", "[GTEST] %s completed %u tests with u% failed test cases.", unit_test.current_test_info()->name(), unit_test.total_test_count(), unit_test.failed_test_case_count());
                }
            }
        };
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ITestEnvironment* DefaultTestEnv();
    }
}

#endif // V_FRAMEWORK_VTEST_VTEST_H