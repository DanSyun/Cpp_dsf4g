#ifndef _INC_SINGLETON_H_
#define _INC_SINGLETON_H_
#pragma once

#include "common_include.h"
template<typename T>
class Singleton
{
public:
    static T* Instance()
    {
        static T _t;
        return &_t;
    }

protected:
    Singleton() {}
    Singleton(const T& t) {}
};

template<typename T>
class TSingleton
{
public:
	static T* Instance(){
		if(NULL == pObj)
		{
			pObj =  new T;
		}
		return pObj;
	}
private:
	static T* pObj;
};

template<typename T>
T* TSingleton<T>::pObj = NULL;

#endif
