package WDM.utils;

import java.util.concurrent.ThreadLocalRandom;

/**
 * Twitter_Snowflake<br>
 * SnowFlake:<br>
 * 0 - 0000000000 0000000000 0000000000 0000000000 0 - 00000 - 00000 - 000000000000 <br>
 * 1-bit identification, since the long basic type is signed in Java, the highest bit is the sign bit, positive numbers are 0, and negative numbers are 1, so id is generally positive, and the highest bit is 0<br>
 * 41-bit time cutoff (millisecond level), note that the 41-bit time cutoff is not the time cutoff that stores the current time, but the difference between the time cutoffs (current time cutoff - start time cutoff)
 * The value obtained), the start time here is generally the time when our id generator starts to be used, which is specified by our program (the startTime property of the IdWorker class in the following program). 41-bit time cutoff, can use 69 years, year T = (1L << 41) / (1000L * 60 * 60 * 24 * 365) = 69<br>
 * 10 data machine bits, which can be deployed on 1024 nodes, including 5 datacenterId and 5 workerId<br>
 * 12-bit sequence, counting in milliseconds, 12-bit counting sequence number supports each node to generate 4096 ID numbers every millisecond (same machine, same time interval)<br>
 * Add up to exactly 64 bits, which is a Long type. <br>
 * The advantage of SnowFlake is that it is sorted by time as a whole, and there is no ID collision in the entire distributed system (distinguished by the data center ID and machine ID), and the efficiency is high. After testing, SnowFlake can generate About 260,000 IDs.
 */
public class UniqueOrderGenerate {

    // ==============================Fields===========================================
    /**
     * starting timestamp (2018-07-03)
     */

    private final long twepoch = 1530607760000L;

    /**
     * workerIdBits
     */
    private final long workerIdBits = 5L;

    /**
     * datacenterIdBits
     */
    private final long datacenterIdBits = 5L;

    /**
     * maxWorkerId: 31 (fast method to compute the max decimal number the binary number can represent)
     */
    private final long maxWorkerId = -1L ^ (-1L << workerIdBits);

    /**
     * maxDatacenterId: 31
     */
    private final long maxDatacenterId = -1L ^ (-1L << datacenterIdBits);

    /**
     * sequenceBits
     */
    private final long sequenceBits = 12L;

    /**
     * workerId shift to the left 12
     */
    private final long workerIdShift = sequenceBits;

    /**
     * datacenterId shift to the left 17(12+5)
     */
    private final long datacenterIdShift = sequenceBits + workerIdBits;

    /**
     * timestamp shift to the left 22(5+5+12)
     */
    private final long timestampLeftShift = sequenceBits + workerIdBits + datacenterIdBits;

    /**
     * sequence mask
     */
    private final long sequenceMask = -1L ^ (-1L << sequenceBits);

    /**
     * workerId ID(0~31)
     */
    private long workerId;

    /**
     * datacenterIdID(0~31)
     */
    private long datacenterId;

    /**
     * sequence in 1 millisecond(0~4095)
     */
    private long sequence = 0L;

    /**
     * lastTimestamp
     */
    private long lastTimestamp = -1L;

    //==============================Constructors=====================================

    /**
     * 构造函数
     *
     * @param workerId (0~31)
     * @param datacenterId (0~31)
     */
    public UniqueOrderGenerate(long workerId, long datacenterId) {
        if (workerId > maxWorkerId || workerId < 0) {
            throw new IllegalArgumentException(String.format("worker Id can't be greater than %d or less than 0", maxWorkerId));
        }
        if (datacenterId > maxDatacenterId || datacenterId < 0) {
            throw new IllegalArgumentException(String.format("datacenter Id can't be greater than %d or less than 0", maxDatacenterId));
        }
        this.workerId = workerId;
        this.datacenterId = datacenterId;
    }

    // ==============================Methods==========================================

    /**
     * Get next ID (thread secure)
     *
     * @return SnowflakeId
     */
    public synchronized long nextId() {
        long timestamp = timeGen();

        if (timestamp < lastTimestamp) {
            throw new RuntimeException(
                    String.format("Clock moved backwards.  Refusing to generate id for %d milliseconds", lastTimestamp - timestamp));
        }

        if (lastTimestamp == timestamp) {
            sequence = (sequence + 1) & sequenceMask;
            if (sequence == 0) {
                timestamp = tilNextMillis(lastTimestamp);
            }
        }
        else {
            sequence = 0L;
        }

        lastTimestamp = timestamp;

        //64-digit ID
        return (((timestamp - twepoch) << timestampLeftShift) //
                | (datacenterId << datacenterIdShift) //
                | (workerId << workerIdShift) //
                | sequence) * 1000 + ThreadLocalRandom.current().nextInt(0, 9999 + 1);
    }

    /**
     * @param lastTimestamp
     * @return timeStampNow
     */
    protected long tilNextMillis(long lastTimestamp) {
        long timestamp = timeGen();
        while (timestamp <= lastTimestamp) {
            timestamp = timeGen();
        }
        return timestamp;
    }

    /**
     * @return currentTimeStamp(ms)
     */
    protected long timeGen() {
        return System.currentTimeMillis();
    }

    //==============================Test=============================================

    /**
     * Test
     */
    public static void main(String[] args) {
        UniqueOrderGenerate idWorker = new UniqueOrderGenerate(0, 0);
        for (int i = 0; i < 1000; i++) {
            long id = idWorker.nextId();
            //System.out.println(Long.toBinaryString(id));
            System.out.println(id);
        }
    }
}