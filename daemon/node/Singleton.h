#ifndef BLUZELLE_SINGLETON_H
#define BLUZELLE_SINGLETON_H

template<typename T>
class Singleton
{
    Singleton(const Singleton&) = delete;
    Singleton& operator= (const Singleton&) = delete;
    /*inline*/ static T * m_instance;

    class MemoryGuard
    {
    public:
        ~MemoryGuard()
        {
            delete m_instance;
            m_instance = nullptr;
        }
    };

protected:
    Singleton() {}
    virtual ~Singleton() {}

public:
    static T& get_instance()
    {
        static MemoryGuard g;
        if (!m_instance)
            {
            m_instance =  new T();
            }
        return *m_instance;
    }
};

template<typename T>
T* Singleton<T>::m_instance = nullptr;

#endif //BLUZELLE_SINGLETON_H
