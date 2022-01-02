#pragma once
#include <vector>
#include <memory>


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


    // 观察者的数量
    auto observers() const {
        return observers_.size();
    }


    auto addObserver(observer_ptr o) {
        observers_.push_back(o);
        return observers_.size() - 1;
    }

    observer_ptr getObserver(unsigned idx) {
        return observers_[idx];
    }

    void eraseObserver(unsigned id) {
        observers_.erase(observers_.begin() + id);
    }

    bool notifyObservers(T... e) {
        for(auto& o : observers_)
            if(!o->update(e...))
                return false;

        return true;
    }


private:
    std::vector<observer_ptr> observers_;
};
