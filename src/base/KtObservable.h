#pragma once
#include <vector>
#include <memory>
#include <atomic>
#include "KuThread.h"


// 自带同步的观察者模式

template<typename ...T>
class KtObserver
{
public:
    virtual bool update(T...) = 0;
};


template<typename ...T>
class KtObservable
{
public:
    using observer_type = KtObserver<T...>;
    using observer_ptr = std::shared_ptr<observer_type>;


    KtObservable() : pauseFlag_(false), actives_(0) {}


    // 观察者的数量
    auto count() const { return observers_.size(); }

    void pushBack(observer_ptr o) { observers_.push_back(o); }
    void pushFront(observer_ptr o) { observers_.insert(observers_.begin(), o); }
    void insert(unsigned idx, observer_ptr o) { observers_.insert(observers_.begin()+idx, o); }

    observer_ptr front() const { return *observers_.begin(); }
    observer_ptr back() const { return observers_.back(); }
    observer_ptr getAt(unsigned idx) { return observers_[idx]; }

    // 获取特定类型的observer
    template<typename T>
    T* get() {
        for (unsigned i = 0; i < observers_.size(); i++) {
            auto obs = dynamic_cast<T*>(observers_[i].get());
            if (obs != nullptr)
                return obs;
        }

        return nullptr;
    }

    void popBack() { observers_.pop_back();  };
    void popFront() { removeAt(0); }
    void removeAt(unsigned idx) { observers_.erase(observers_.begin() + idx); }
    void remove(observer_type* obs) {
        for (auto iter = observers_.cbegin(); iter != observers_.cend(); ++iter)
            if (iter->get() == obs) {
                observers_.erase(iter);
                break;
            }
    }
    void remove(observer_ptr obs) { remove(obs.get()); }

    // 删除特定类型的observer
    template<typename T>
    void remove() {
        for (auto iter = observers_.cbegin(); iter != observers_.cend();) {
            auto obs = dynamic_cast<T*>(iter->get());
            if (obs != nullptr)
                iter = observers_.erase(iter);
            else
                ++iter;
        }
    }


    bool notify(T... e) {
        if (pauseFlag_)
            return false;

        ++actives_;

        for(auto& o : observers_)
            if (!o->update(e...)) {
                --actives_;
                return false;
            }

        --actives_;

        return true;
    }


    // 终止向观察者notify
    // @wait: true则等待所有活动的notify结束
    // @t: 仅当wait=true有效，表示等待轮询时间，单位为s
    void freeze(bool wait = true, float t = 0.05) {
        pauseFlag_ = true;
        while(0 != (int)actives_)
            KuThread::wait(t);
    }


    // 重新激活向观察者发送notify
    void reset() {
        pauseFlag_ = false;
        actives_ = 0;
    }

private:
    std::vector<observer_ptr> observers_;

    std::atomic_bool pauseFlag_;
    std::atomic_int actives_; // 
};
