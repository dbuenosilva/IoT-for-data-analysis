SELECT 
	deviceId,
    temperature,
	humidity,
    msgCount,
    EventProcessedUtcTime ,
    PartitionId,
    EventEnqueuedUtcTime
INTO
    [FFM-IoT-Staging]
FROM
    [FFM-devices]