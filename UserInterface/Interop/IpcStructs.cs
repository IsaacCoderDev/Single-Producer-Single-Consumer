using System.Runtime.InteropServices;

namespace UserInterface.Interop;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct IpcMessage
{
    public long TimestampNs;
    public double Price;
    public double Quantity;
    public int InstrumentId;
    
    private unsafe fixed byte _padding[36]; 
}

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct RingBufferHeader
{
    public volatile int Head;
    public volatile int Tail;
    
    private unsafe fixed byte _padding[56]; 
}