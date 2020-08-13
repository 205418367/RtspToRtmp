#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

#include <list>
#include <mutex>
#include <thread>

template <typename T>
class PacketQueue
{
public:
    PacketQueue();
    ~PacketQueue();
    void AddData(T& data);
    bool GetData(T& data);
    void GetDataList(std::list<T>& list);
    int  Size();
    void Clear();

private:
    bool IsEmpty();
    bool IsFull();
    int  Capacity();

private:
    std::mutex list_mutex_;
    std::list<T> data_list_;
    int capacity_;//容量
    int size_;//当前大小
};



template <typename T> 
PacketQueue<T>::PacketQueue()
{
    capacity_ = 200;
    size_ = 0;
}

template <typename T> 
PacketQueue<T>::~PacketQueue()
{

}

template <typename T> 
void PacketQueue<T>::AddData(T& data)
{
    do{
        std::unique_lock<std::mutex> locker(list_mutex_);
        if(!IsFull()){
            data_list_.push_back(data);
            ++size_;
        }
    }while(false);   
}

template <typename T> 
bool PacketQueue<T>::GetData(T& data)
{
    bool ret = false;
    do
    {
        std::unique_lock<std::mutex> locker(list_mutex_);
        if(!IsEmpty()){
            data = data_list_.front();
            data_list_.pop_front();
            ret = true;
            if(size_ >= 1){
                --size_;
            }
        }
    } while (false);

    return ret;
}

template <typename T> 
void PacketQueue<T>::GetDataList(std::list<T>& list)
{
    do{
        std::unique_lock<std::mutex> locker(list_mutex_);
        list = data_list_;
    }while(false);
}

template <typename T> 
void PacketQueue<T>::Clear()
{
    do
    {
        std::unique_lock<std::mutex> locker(list_mutex_);
        data_list_.clear();
        size_ = 0;
    } while (false);
}

template <typename T> 
bool PacketQueue<T>::IsEmpty()
{
    return data_list_.empty();
}

template <typename T> 
bool PacketQueue<T>::IsFull()
{
    return data_list_.size() == capacity_;
}

template <typename T> 
int  PacketQueue<T>::Size()
{
    std::unique_lock<std::mutex> locker(list_mutex_);
    return data_list_.size();
}

template <typename T> 
int  PacketQueue<T>::Capacity()
{
    std::unique_lock<std::mutex> locker(list_mutex_);
    return capacity_;
}

#endif