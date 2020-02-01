#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <vector>

#include <libaio.h>

class AIOContext
{
public:
    class AioEvent;
    
    AIOContext( size_t queue_depth );
    
    ~AIOContext();
    
    void queueWrite( int fd, uint8_t* data, std::size_t size, std::size_t offset );
    
    void queueRead(  int fd, uint8_t* data, std::size_t size, std::size_t offset  );
    
    std::size_t submitRequests();
    
    std::vector< Event > collectRequests( std::size_t min_requests, std::size_t max_requests );
    
    class AioEvent {
    friend AIOContext;
        
    public:
        enum class AioEventType
        {
            read,
            write
        };
        
        ~AioEvent();
        uint8_t* getData();
        AioEventType getType();
        
    private:
        AioEvent( io_event event );
        
        io_event event;
        AioEventType type;
    };
    
    
private:
    std::size_t remainingAIOQueueDepth();
    std::vector< iocb* > getCurrentRequests( std::size_t request_count );
    
    io_context_t aio_ctx;
    std::size_t queue_depth;
    
    std::size_t inflight_requests;
    std::list< iocb* > queued_requests;
};