using System.ComponentModel;
using System.Runtime.CompilerServices;
using UserInterface.Interop;

namespace QuantUI.ViewModels;

public class DashboardViewModel : INotifyPropertyChanged
{
    private readonly ZeroCopyIpcReader _reader;
    private readonly double _backendPrice;
    private double _latestPrice;
    
    public double LatestPrice
    {
        get => _latestPrice;
        set
        {
            _latestPrice = value;
            
            OnPropertyChanged();
        }
    }

    public DashboardViewModel()
    {
        _reader = new ZeroCopyIpcReader("quant_ring_buffer", 1024 * 1024);
        
        Task.Run(IngestionLoop);
        
        Task.Run(RenderLoop);
    }

    private void IngestionLoop()
    {
        double tempPrice = 0;
        
        while (true)
        {
            while (_reader.TryRead(out var msg))
            {
                tempPrice = msg.Price;
            }
            
            Thread.SpinWait(10); 
        }
    }

    private async Task RenderLoop()
    {
        using var timer = new PeriodicTimer(TimeSpan.FromMilliseconds(16.6));
        
        while (await timer.WaitForNextTickAsync())
        {
            MainThread.BeginInvokeOnMainThread(() =>
            {
                LatestPrice = _backendPrice; 
            });
        }
    }

    public event PropertyChangedEventHandler PropertyChanged;
    protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}