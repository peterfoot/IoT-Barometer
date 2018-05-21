#r "Newtonsoft.Json"

using System;
using System.Configuration;
using System.Text;
using Microsoft.Azure.Devices;
using Newtonsoft.Json;

public class WeatherReport
{
    [JsonProperty("temp")]
    public float Temperature{get;set;}
    [JsonProperty("pressure")]
    public float Pressure{get;set;}
    [JsonProperty("setPressure")]
    public float SetPressure{get;set;}
    [JsonProperty("humidity")]
    public float Humidity{get;set;}
    [JsonProperty("wind")]
    public string WindDirection {get;set;}
}

public static void Run(string myEventHubMessage, TraceWriter log)
{
    string json = myEventHubMessage;
    log.Info(json);
    WeatherReport wr = JsonConvert.DeserializeObject<WeatherReport>(json);
    log.Info($"C# Event Hub trigger function processed a message: {myEventHubMessage}");

    string message = "Fair";

    if(wr.Pressure < (wr.SetPressure - 5))
    {
        if(wr.Pressure < 980)
        {
            message = "Storm warning";
        }
        else if(wr.Pressure < 1000)
        {
            message = "Rain warning";
        }
        else if(wr.Pressure < 1020)
        {
            message = "Change warning";
        }
    }
    else
    {
        if(wr.Pressure < 965)
        {
            message = "Stormy";
        }
        else if(wr.Pressure < 982)
        {
            message = "Rain";
        }
        else if(wr.Pressure < 1016)
        {
            message = "Change";
        }
        else if(wr.Pressure < 1032)
        {
            message = "Fair";
        }
        else
        {
            message = "Very Dry";
        }
    }
        
    // Send back to device
        
    string jsonMessage = $"{{\"alert\": \"{message}\"}}";
    string deviceId = "AZ3166";
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
