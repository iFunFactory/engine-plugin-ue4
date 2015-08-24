// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include <map>
#include <list>


namespace Fun
{
    typedef void(*TimerEventHandler)(void* param);

    // Timer
    class FunapiTimer
    {
    public:
        FunapiTimer();

        inline void AddTimer (string name, float delay, bool loop, TimerEventHandler callback, void* param = NULL)
        {
            AddTimer(name, 0, delay, loop, callback, param);
        }

        inline void AddTimer (string name, float start, TimerEventHandler callback, void* param = NULL)
        {
            AddTimer(name, start, 0, false, callback, param);
        }

        void AddTimer (string name, float start, float delay, bool loop, TimerEventHandler callback, void* param = NULL);

        void KillTimer (string name);

        inline bool ContainTimer (string name)
        {
            return timer_list_.find(name) != timer_list_.end();
        }

        inline void Clear()
        {
            is_all_clear_ = true;
        }

        void Update (float deltaTime);

    private:
        // Removes timer
        void CheckRemoveList();


        class Event
        {
        public:
            string name;
            bool loop;
            float remaining;
            float interval;
            TimerEventHandler callback;
            void* param;

            Event (string name, float start, float delay, bool loop, TimerEventHandler callback, void* param)
            {
                this->name = name;
                this->loop = loop;
                this->interval = delay;
                this->remaining = start;
                this->callback = callback;
                this->param = param;
            }
        };


        std::map<string, Event*> timer_list_;
        std::map<string, Event*> pending_list_;
        std::list<string> remove_list_;
        bool is_all_clear_;
    };

} // namespace Fun
