#include <vcore/detector/detector.h>
#include <vcore/detector/detector_bus.h>

#include <vcore/std/parallel/mutex.h>
#include <vcore/std/parallel/lock.h>
#include <vcore/math/crc.h>


namespace V
{
    namespace Debug
    {
        class DetectorManagerImpl
            : public DetectorManager
        {
        public:
            V_CLASS_ALLOCATOR(DetectorManagerImpl, OSAllocator, 0);

            typedef forward_list<DetectorSession>::type SessionListType;
            SessionListType m_sessions;
            typedef vector<Detector*>::type DetectorArrayType;
            DetectorArrayType m_drillers;

            ~DetectorManagerImpl() override;

            void Register(Detector* factory) override;
            void Unregister(Detector* factory) override;

            void FrameUpdate() override;

            DetectorSession*     Start(DetectorOutputStream& output, const DetectorListType& drillerList, int numFrames = -1) override;
            void                Stop(DetectorSession* session) override;

            int                 GetNumDetectors() const override  { return static_cast<int>(m_drillers.size()); }
            Detector*            GetDetector(int index) override   { return m_drillers[index]; }
        };

        //////////////////////////////////////////////////////////////////////////
        // Detector

        //=========================================================================
        // Register
        //=========================================================================
        V::u32 Detector::GetId() const
        {
            return V::Crc32(GetName());
        }

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Detector Manager

        //=========================================================================
        // Register
        //=========================================================================
        DetectorManager* DetectorManager::Create(/*const Descriptor& desc*/)
        {
            const bool createAllocator = !V::AllocatorInstance<OSAllocator>::IsReady();
            if (createAllocator)
            {
                V::AllocatorInstance<OSAllocator>::Create();
            }

            DetectorManagerImpl* impl = vnew DetectorManagerImpl;
            impl->m_ownsOSAllocator = createAllocator;
            return impl;
        }

        //=========================================================================
        // Register
        //=========================================================================
        void DetectorManager::Destroy(DetectorManager* manager)
        {
            const bool allocatorCreated = manager->m_ownsOSAllocator;
            delete manager;
            if (allocatorCreated)
            {
                V::AllocatorInstance<OSAllocator>::Destroy();
            }
        }

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // DetectorManagerImpl

        //=========================================================================
        // ~DetectorManagerImpl
        //=========================================================================
        DetectorManagerImpl::~DetectorManagerImpl()
        {
            while (!m_sessions.empty())
            {
                Stop(&m_sessions.front());
            }

            while (!m_drillers.empty())
            {
                Detector* driller = m_drillers[0];
                Unregister(driller);
                delete driller;
            }
        }

        //=========================================================================
        // Register
        //=========================================================================
        void
        DetectorManagerImpl::Register(Detector* driller)
        {
            V_Assert(driller, "You must provide a valid factory!");
            for (size_t i = 0; i < m_drillers.size(); ++i)
            {
                if (m_drillers[i]->GetId() == driller->GetId())
                {
                    V_Error("Debug", false, "Detector with id %08x has already been registered! You can't have two factory instances for the same driller type", driller->GetId());
                    return;
                }
            }
            m_drillers.push_back(driller);
        }

        //=========================================================================
        // Unregister
        //=========================================================================
        void
        DetectorManagerImpl::Unregister(Detector* driller)
        {
            V_Assert(driller, "You must provide a valid factory!");
            for (DetectorArrayType::iterator iter = m_drillers.begin(); iter != m_drillers.end(); ++iter)
            {
                if ((*iter)->GetId() == driller->GetId())
                {
                    m_drillers.erase(iter);
                    return;
                }
            }

            V_Error("Debug", false, "Failed to find driller factory with id %08x", driller->GetId());
        }

        //=========================================================================
        // FrameUpdate
        //=========================================================================
        void
        DetectorManagerImpl::FrameUpdate()
        {
            if (m_sessions.empty())
            {
                return;
            }

            VStd::lock_guard<DetectorEventBusMutex::MutexType> lock(DetectorEventBusMutex::GetMutex());  ///< Make sure no driller is writing to the stream
            for (SessionListType::iterator sessionIter = m_sessions.begin(); sessionIter != m_sessions.end(); )
            {
                DetectorSession& s = *sessionIter;

                // tick the drillers directly if they care.
                for (size_t i = 0; i < s.detectors.size(); ++i)
                {
                    s.detectors[i]->Update();
                }

                s.output->EndTag(V_CRC("Frame", 0xb5f83ccd));

                s.output->OnEndOfFrame();

                s.curFrame++;

                if (s.numFrames != -1)
                {
                    if (s.curFrame == s.numFrames)
                    {
                        Stop(&s);
                        continue;
                    }
                }

                s.output->BeginTag(V_CRC("Frame", 0xb5f83ccd));
                s.output->Write(V_CRC("FrameNum", 0x85a1a919), s.curFrame);

                ++sessionIter;
            }
        }

        //=========================================================================
        // Start
        //=========================================================================
        DetectorSession*
        DetectorManagerImpl::Start(DetectorOutputStream& output, const DetectorListType& drillerList, int numFrames)
        {
            if (drillerList.empty())
            {
                return nullptr;
            }

            m_sessions.push_back();
            DetectorSession& s = m_sessions.back();
            s.curFrame = 0;
            s.numFrames = numFrames;
            s.output = &output;

            s.output->WriteHeader(); // first write the header in the stream

            s.output->BeginTag(V_CRC("StartData", 0xecf3f53f));
            s.output->Write(V_CRC("Platform", 0x3952d0cb), (unsigned int)g_currentPlatform);
            for (DetectorListType::const_iterator iDetector = drillerList.begin(); iDetector != drillerList.end(); ++iDetector)
            {
                const DetectorInfo& di = *iDetector;
                s.output->BeginTag(V_CRC("Detector", 0xa6e1fb73));
                s.output->Write(V_CRC("Name", 0x5e237e06), di.id);
                for (int iParam = 0; iParam < (int)di.params.size(); ++iParam)
                {
                    s.output->BeginTag(V_CRC("Param", 0xa4fa7c89));
                    s.output->Write(V_CRC("Name", 0x5e237e06), di.params[iParam].name);
                    s.output->Write(V_CRC("Description", 0x6de44026), di.params[iParam].desc);
                    s.output->Write(V_CRC("Type", 0x8cde5729), di.params[iParam].type);
                    s.output->Write(V_CRC("Value", 0x1d775834), di.params[iParam].value);
                    s.output->EndTag(V_CRC("Param", 0xa4fa7c89));
                }
                s.output->EndTag(V_CRC("Detector", 0xa6e1fb73));
            }
            s.output->EndTag(V_CRC("StartData", 0xecf3f53f));

            s.output->BeginTag(V_CRC("Frame", 0xb5f83ccd));
            s.output->Write(V_CRC("FrameNum", 0x85a1a919), s.curFrame);

            {
                VStd::lock_guard<DetectorEventBusMutex::MutexType> lock(DetectorEventBusMutex::GetMutex());  ///< Make sure no driller is writing to the stream
                for (DetectorListType::const_iterator iDetector = drillerList.begin(); iDetector != drillerList.end(); ++iDetector)
                {
                    Detector* driller = nullptr;
                    const DetectorInfo& di = *iDetector;
                    for (size_t iDesc = 0; iDesc < m_drillers.size(); ++iDesc)
                    {
                        if (m_drillers[iDesc]->GetId() == di.id)
                        {
                            driller = m_drillers[iDesc];
                            V_Assert(driller->m_output == nullptr, "Detector with id %08x is already have an output stream %p (currently we support only 1 at a time)", di.id, driller->m_output);
                            driller->m_output = &output;
                            driller->Start(di.params.data(), static_cast<unsigned int>(di.params.size()));
                            s.detectors.push_back(driller);
                            break;
                        }
                    }
                    V_Warning("Detector", driller != nullptr, "We can't start a driller with id %d!", di.id);
                }
            }
            return &s;
        }


        //=========================================================================
        // Stop
        //=========================================================================
        void
        DetectorManagerImpl::Stop(DetectorSession* session)
        {
            SessionListType::iterator iter;
            for (iter = m_sessions.begin(); iter != m_sessions.end(); ++iter)
            {
                if (&*iter == session)
                {
                    break;
                }
            }

            V_Assert(iter != m_sessions.end(), "We did not find session ID 0x%08x in the list!", session);
            if (iter != m_sessions.end())
            {
                DetectorSession& s = *session;

                {
                    VStd::lock_guard<DetectorEventBusMutex::MutexType> lock(DetectorEventBusMutex::GetMutex());
                    for (size_t i = 0; i < s.detectors.size(); ++i)
                    {
                        s.detectors[i]->Stop();
                        s.detectors[i]->m_output = nullptr;
                    }
                }
                s.output->EndTag(V_CRC("Frame", 0xb5f83ccd));
                m_sessions.erase(iter);
            }
        }
    } // namespace Debug
} // namespace V