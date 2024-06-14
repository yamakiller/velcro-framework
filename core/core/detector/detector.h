#ifndef V_FRAMEWORK_CORE_DETECTOR_DETECTOR_H
#define V_FRAMEWORK_CORE_DETECTOR_DETECTOR_H

namespace VStd {
    class mutex;
}

namespace V {
    namespace Debug {
        class Detector {
            //friend class DrillerManagerImpl;

        public:
            struct Param {
                enum Type {
                    PT_BOOL,
                    PT_INT,
                    PT_FLOAT
                };
                const char* desc;
                //V::u32 name;
                int    type;
                int    value;
            };
        }
    }
}

#endif // V_FRAMEWORK_CORE_DETECTOR_DETECTOR_H