#r "Microsoft.ServiceBus"
#r "Newtonsoft.Json"

using System;
using System.Configuration;
using System.Text;
using Microsoft.Azure.Devices;
using Microsoft.ServiceBus.Messaging;
using Newtonsoft.Json;

public class WeatherReport
{
    [JsonProperty("temp")]
    public float Temperature{get;set;}
    [JsonProperty("pressure")]
    public float Pressure{get;set;}
    [JsonProperty("markedPressure")]
    public float MarkedPressure{get;set;}
    [JsonProperty("humidity")]
    public float Humidity{get;set;}
    [JsonProperty("wind")]
    public string WindDirection {get;set;}
}

public static void Run(EventData myEventHubMessage, TraceWriter log)
{
    object id = null;
    
    try{
        id = myEventHubMessage.SystemProperties["iothub-connection-device-id"];
    }
    catch{}

    string deviceId = id == null ? string.Empty : id.ToString();
    //log.Info(deviceId);
    
    string json = System.Text.Encoding.UTF8.GetString(myEventHubMessage.GetBytes());
    //log.Info(json);
    WeatherReport wr = JsonConvert.DeserializeObject<WeatherReport>(json);

    string message = "Fair";

    if(wr.Pressure < (wr.MarkedPressure - 5))
    {
        if(wr.Pressure < 1000)
        {
            message = "Storm warning";
        }
        else if(wr.Pressure < 1010)
        {
            message = "Rain warning";
        }
        else if(wr.Pressure < 1015)
        {
            message = "Change warning";
        }
    }
    else
    {
        if(wr.Pressure < 990)
        {
            message = "Stormy";
        }
        else if(wr.Pressure < 1005)
        {
            message = "Rain";
        }
        else if(wr.Pressure < 1010)
        {
            message = "Change";
        }
        else if(wr.Pressure < 1015)
        {
            message = "Fair";
        }
        else
        {
            message = "Very Dry";
        }
    }
        
    // Send back to device
    if(!string.IsNullOrEmpty(deviceId))    
    {
        string jsonMessage = $"{{\"alert\": \"{message}\"}}";
        try
        {
            string connectionString = ConfigurationManager.AppSettings["iotHubConnectionString"];
            using(ServiceClient serviceClient = ServiceClient.CreateFromConnectionString(connectionString))
            {
                Message commandMessage = new Message(Encoding.UTF8.GetBytes(jsonMessage));
                serviceClient.SendAsync(deviceId, commandMessage).Wait();
            }
        }
        catch(Exception ex)
        {
            throw new Exception($"Failed to send C2D message: {ex.Message}");
        }
    }     
}
