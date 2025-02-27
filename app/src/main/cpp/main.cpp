#include <jni.h>
#include <game-activity/GameActivity.cpp>
#include <game-activity/native_app_glue/android_native_app_glue.c>
#include <game-text-input/gametextinput.cpp>
#include "CRenderer.h"
#include "Common.h"

extern "C"
{
    /*!
     * Handles commands sent to this Android application
     * @param vApp the app the commands are coming from
     * @param vCmd the command to handle
     */
    void handleCmd(android_app* vApp, int32_t vCmd)
    {
        switch (vCmd)
        {
            case APP_CMD_INIT_WINDOW:
                // A new window is created, associate a renderer with it. You may replace this with a
                // "game" class if that suits your needs. Remember to change all instances of userData
                // if you change the class here as a reinterpret_cast is dangerous this in the
                // android_main function and the APP_CMD_TERM_WINDOW handler case.
                vApp->userData = new hiveVG::CRenderer(vApp);
                break;
            case APP_CMD_TERM_WINDOW:
                // The window is being destroyed. Use this to clean up your userData to avoid leaking
                // resources.
                //
                // We have to check if userData is assigned just in case this comes in really quickly
                if (vApp->userData)
                {
                    auto *pRenderer = reinterpret_cast<hiveVG::CRenderer*>(vApp->userData);
                    vApp->userData = nullptr;
                    delete pRenderer;
                }
                break;
            default:
                break;
        }
    }

    /*!
     * Enable the motion events you want to handle; not handled events are
     * passed back to OS for further processing. For this example case,
     * only pointer and joystick devices are enabled.
     *
     * @param vMotionEvent the newly arrived GameActivityMotionEvent.
     * @return true if the event is from a pointer or joystick device,
     *         false for all other input devices.
     */
    bool motion_event_filter_func(const GameActivityMotionEvent* vMotionEvent)
    {
        auto sourceClass = vMotionEvent->source & AINPUT_SOURCE_CLASS_MASK;
        return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
                sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
    }

    void android_main(struct android_app* vApp)
    {
        vApp->onAppCmd = handleCmd;

        // Set input event filters (set it to NULL if the app wants to process all inputs).
        // Note that for key inputs, this example uses the default default_key_filter()
        // implemented in android_native_app_glue.c.
        // Set filters for touch events in your application
        android_app_set_motion_event_filter(vApp, motion_event_filter_func);

        do
        {
            // Process all pending events before running game logic.
            bool Done = false;
            while (!Done) // Polling Events
            {
                // 0 is non-blocking.
                int Timeout = 0;
                int Events;
                android_poll_source* pSource;
                int Result = ALooper_pollOnce(Timeout, nullptr, &Events,
                                              reinterpret_cast<void**>(&pSource));
                switch (Result)
                {
                    case ALOOPER_POLL_TIMEOUT:
                        [[clang::fallthrough]];
                    case ALOOPER_POLL_WAKE:
                        // No Events occurred before the Timeout or explicit wake. Stop checking for Events.
                        Done = true;
                        break;
                    case ALOOPER_EVENT_ERROR:
                        LOG_ERROR(hiveVG::TAG_KEYWORD::MAIN_TAG, "ALooper_pollOnce returned an error");
                        break;
                    case ALOOPER_POLL_CALLBACK:
                        break;
                    default:
                        if (pSource)
                        {
                            pSource->process(vApp, pSource);
                        }
                }
            }

            if (vApp->userData)
            {
                auto *pRenderer = reinterpret_cast<hiveVG::CRenderer*>(vApp->userData);
                pRenderer->render();
            }
        } while (!vApp->destroyRequested);
    }
}