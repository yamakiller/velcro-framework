#ifndef V_FRAMEWORK_CORE_DETECTOR_DETECTOR_HZ
#define V_FRAMEWORK_CORE_DETECTOR_DETECTOR_H

#include <vcore/detector/stream.h>

namespace VStd {
   class mutex;
}

namespace V {
    namespace Debug {
        class DetectorOutputStream;

        /**
         * Detector base class. Every detector should inherit from this class.
         * When a detector is need to start outputting data
         * the DetectorManager will call Detector::Start() so the detector
         * can output the initial state for all reported entities.
         * The same applies for the Stop.
         * Depending on the type of your detector you might choose to collect state
         * even before the detector has started. This of course should be a fast as
         * possible, as we don't want to burden engine systems and it's highly recommended
         * that you use configuration parameters to change that behavior as not all detectors
         * are used on a daily basis.
         * All detectors should use DebugAllocators (V_CLASS_ALLOCATOR(Detector,OSAllocator,0))
         * and they should use 'vnew' to create one, as by default if you don't unregister a
         * a detector, the manager will use "delete" to delete them.
         *
         * IMPORTANT: Detector systems works OUTSIDE engine systems, you should NOT use SystemAllocator or any other engine systems
         * as they might be drilled or not available at the moment.
         */
        class Detector
        {
            friend class DetectorManagerImpl;

        public:
            struct Param {
                enum Type {
                    PT_BOOL,
                    PT_INT,
                    PT_FLOAT
                };
                const char* desc;
                u32 name;
                int type;
                int value;
            };

            Detector()
                : m_output(NULL) {}
            virtual ~Detector() {}

            /// Returns the detector ID Crc32 of the name (Crc32(GetName())
            V::u32                 GetId() const;
            /// Detector group name, used only for organizational purpose
            virtual const char*     GroupName() const = 0;
            /// Unique name of the Detector, detector ID is the Crc of the name
            virtual const char*     GetName() const = 0;
            virtual const char*     GetDescription() const = 0;
            // @{ Managing the list of supported detector parameters.
            virtual int             GetNumParams() const        { return 0; }
            virtual const Param*    GetParam(int index) const   { (void)index; return NULL; }

        protected:
            Detector& operator=(const Detector&);

            /// Called by DetectorManager
            virtual void Start(const Param* params = NULL, int numParams = 0)   { (void)params; (void)numParams; }
            /// Called by DetectorManager
            virtual void Stop()     {}
            /// Called every frame by DetectorManger (while the detector is started)
            virtual void Update()   {}

            DetectorOutputStream*    m_output;       ///< Session output stream.
        };

        /**
         * Stores the information while an active
         * detector(s) session is running.
         */
        struct DetectorSession
        {
            int numFrames;
            int curFrame;
            typedef vector<Detector*>::type DetectorArrayType;
            DetectorArrayType         detectors;
            DetectorOutputStream*     output;
        };

        /**
         * Detector manager will manage all active detector sessions and detector factories. Generally you will never
         * need more than one detector manger.
         * IMPORTANT: Detector systems works OUTSIDE engine systems, you should NOT use SystemAllocator or any other engine systems
         * as they might be drilled or not available at the moment.
         */
        class DetectorManager {
            friend class DetectorRemoteServer;
        public:
            struct DetectorInfo {
                V::u32     id;
                vector<Detector::Param>::type params;
            };
            typedef forward_list<DetectorInfo>::type DetectorListType;

            virtual ~DetectorManager() {}

            static DetectorManager*  Create(/*const Descriptor& desc*/);
            static void              Destroy(DetectorManager* manager);

            virtual void             Register(Detector* detector) = 0;
            virtual void             Unregister(Detector* detector) = 0;

            virtual void             FrameUpdate() = 0;

            virtual DetectorSession* Start(DetectorOutputStream& output, const DetectorListType& detectorList, int numFrames = -1) = 0;
            virtual void             Stop(DetectorSession* session) = 0;

            virtual int              GetNumDetectors() const  = 0;
            virtual Detector*        GetDetector(int index) = 0;

        private:
            // If the manager created the allocator, it should destroy it when it gets destroyed
            bool m_ownsOSAllocator = false;
        };
    } // namespace Debug
} // namespace V

#endif // V_FRAMEWORK_CORE_DETECTOR_DETECTOR_H