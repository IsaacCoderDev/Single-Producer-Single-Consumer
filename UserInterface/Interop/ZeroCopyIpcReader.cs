using System.IO.MemoryMappedFiles;
using System.Runtime.CompilerServices;

namespace UserInterface.Interop;

public unsafe class ZeroCopyIpcReader : IDisposable
{
    private readonly MemoryMappedFile _mmf;
    private readonly MemoryMappedViewAccessor _accessor;
    private readonly byte* _basePointer;
    private readonly RingBufferHeader* _header;
    private readonly IpcMessage* _messages;
    private readonly int _capacity;

    public ZeroCopyIpcReader(string shmName, int capacity)
    {
        _capacity = capacity;
        long totalSize = sizeof(RingBufferHeader) + (sizeof(IpcMessage) * capacity);

        _mmf = MemoryMappedFile.CreateFromFile(
            $"/dev/shm/{shmName}", 
            FileMode.Open, 
            null, 
            totalSize, 
            MemoryMappedFileAccess.ReadWrite);

        _accessor = _mmf.CreateViewAccessor(0, totalSize, MemoryMappedFileAccess.ReadWrite);
        
        _accessor.SafeMemoryMappedViewHandle.AcquirePointer(ref _basePointer);

        _header = (RingBufferHeader*)_basePointer;
        _messages = (IpcMessage*)(_basePointer + sizeof(RingBufferHeader));
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool TryRead(out IpcMessage message)
    {
        int tail = _header->Tail;
        
        Thread.MemoryBarrier(); 
        
        if (tail == _header->Head)
        {
            message = default;
            return false; // Queue is empty
        }

        message = _messages[tail];

        _header->Tail = (tail + 1) % _capacity;
        
        return true;
    }

    public void Dispose()
    {
        _accessor.SafeMemoryMappedViewHandle.ReleasePointer();
        _accessor.Dispose();
        _mmf.Dispose();
    }
}