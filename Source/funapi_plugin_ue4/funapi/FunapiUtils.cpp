// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"
#include "FunapiUtils.h"


namespace Fun
{
    FunapiTimer::FunapiTimer()
        : is_all_clear_(false)
    {
    }

    void FunapiTimer::AddTimer(string name, float start, float delay, bool loop, TimerEventHandler callback, void* param/* = NULL*/)
    {
        if (callback == NULL)
        {
            LOG1("AddTimer - '%s' timer's callback is NULL.", *FString(name.c_str()));
            return;
        }

        if (timer_list_.find(name) != timer_list_.end() || pending_list_.find(name) != pending_list_.end())
        {
            std::list<string>::iterator itr = std::find(remove_list_.begin(), remove_list_.end(), name);
            if (itr != remove_list_.end())
            {
                LOG1("AddTimer - '%s' timer already exists.", *FString(name.c_str()));
                return;
            }
        }

        pending_list_[name] = new Event(name, start, delay, loop, callback, param);
        LOG1("AddTimer - '%s' timer added.", *FString(name.c_str()));
    }

    void FunapiTimer::KillTimer(string name)
    {
        {
            std::map<string, Event*>::iterator itr = pending_list_.find(name);
            if (itr != pending_list_.end())
            {
                pending_list_.erase(itr);
                LOG1("KillTimer - '%s' timer removed.", *FString(name.c_str()));
                return;
            }
        }

        {
            std::list<string>::iterator itr = std::find(remove_list_.begin(), remove_list_.end(), name);
            if (itr == remove_list_.end())
            {
                remove_list_.push_back(name);
                LOG1("KillTimer - '%s' timer removed.", *FString(name.c_str()));
            }
        }
    }

    void FunapiTimer::Update(float deltaTime)
    {
        if (is_all_clear_)
        {
            timer_list_.clear();
            pending_list_.clear();
            remove_list_.clear();
            is_all_clear_ = false;
            return;
        }

        CheckRemoveList();

        // Adds timer
        if (pending_list_.size() > 0)
        {
            std::map<string, Event*>::iterator itr = pending_list_.begin();
            while (itr != pending_list_.end())
            {
                timer_list_[itr->first] = itr->second;
                ++itr;
            }

            pending_list_.clear();
        }

        if (timer_list_.size() <= 0)
            return;

        // Updates timer
        std::map<string, Event*>::iterator itr = timer_list_.begin();
        while (itr != timer_list_.end())
        {
            Event* e = itr->second;
            e->remaining -= deltaTime;
            if (e->remaining <= 0)
            {
                e->callback(e->param);

                if (e->loop)
                    e->remaining = e->interval;
                else
                    remove_list_.push_back(e->name);
            }
        }

        CheckRemoveList();
    }

    // Removes timer
    void FunapiTimer::CheckRemoveList()
    {
        if (remove_list_.size() <= 0)
            return;

        std::list<string>::iterator itr = remove_list_.begin();
        while (itr != remove_list_.end())
        {
            std::map<string, Event*>::iterator fitr = timer_list_.find(*itr);
            if (fitr != timer_list_.end())
            {
                timer_list_.erase(fitr);
                continue;
            }

            fitr = pending_list_.find(*itr);
            if (fitr != pending_list_.end())
            {
                pending_list_.erase(fitr);
            }
        }

        remove_list_.clear();
    }

} // namespace Fun
