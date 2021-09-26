
CREATE TABLE [dbo].[FFM_Devices_Events] (
	deviceId nvarchar(254) NULL,
    temperature int NULL,
	humidity int NULL,
    msgCount int NULL,
    EventProcessedUtcTime nvarchar(254) NULL,
    PartitionId nvarchar(254) NULL,
    EventEnqueuedUtcTime nvarchar(254) NULL,
    IoTHub  nvarchar(254) NULL 
) 