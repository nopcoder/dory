## Status Monitoring

Bruce provides a web-based management interface.  The default port number is
9090, but can be changed as described
[here](https://github.com/tagged/bruce/blob/master/doc/detailed_config.md).
Assuming that Bruce is running on a system named `example`, your web browser
will show the following when directed at `http://example:9090`:

![Bruce management interface](https://github.com/tagged/bruce/blob/master/doc/web_interface.jpg?raw=true)

### Counter Reporting

If you click on `get counter values` (i.e. send an HTTP GET to
`http://example:9090/sys/counters`), you will get a page that looks something
like this:

```
now=1408656417 Thu Aug 21 14:26:57 2014
since=1408585285 Wed Aug 20 18:41:25 2014
pid=14246
version=1.0.6.70.ga324763

[bruce/web_interface.cc, 43].MongooseUrlDecodeError=0
[bruce/web_interface.cc, 42].MongooseUnknownException=0
[bruce/web_interface.cc, 41].MongooseStdException=0
[bruce/web_interface.cc, 40].MongooseHttpRequest=1582
[bruce/web_interface.cc, 39].MongooseGetMsgStatsRequest=0
[bruce/web_interface.cc, 38].MongooseGetMetadataFetchTimeRequest=0
[bruce/web_interface.cc, 37].MongooseGetDiscardsRequest=790
[bruce/web_interface.cc, 36].MongooseGetCountersRequest=792
[bruce/web_interface.cc, 35].MongooseEventLog=0
[bruce/msg.cc, 25].MsgUnprocessedDestroy=0
[bruce/msg.cc, 24].MsgDestroy=26934272
[bruce/msg.cc, 23].MsgCreate=26934440
[bruce/util/pause_button.cc, 14].PauseStarted=0
[bruce/msg_dispatch/sender.cc, 53].SendProduceRequestOk=1015702
(remaining output omitted)
```

In addition to the counter values, the above output shows the time when the
counter report was created (1408656417 seconds since the epoch), the time when
Bruce started running (1408585285 seconds since the epoch), Bruce's process ID
which is 14246, and Bruce's version which is `1.0.6.70.ga324763`.  The counter
values track various events inside Bruce, and can be used for health monitoring
and troubleshooting.  For example, `src/bruce/scripts/bruce_counters.py` is a
Nagios script that monitors Bruce's counters and reports problems.  Details on
the meanings of some of the more interesting counters are provided
[here](https://github.com/tagged/bruce/blob/master/doc/troubleshooting.md).
Also, you can look in Bruce's source code to see what a counter indicates.  For
instance, near the top of `src/bruce/msg.cc` you will see the following
definitions:

```C++
SERVER_COUNTER(MsgCreate);
SERVER_COUNTER(MsgDestroy);
SERVER_COUNTER(MsgUnprocessedDestroy);
```

These create the counters of the same names shown above.  Then you can look for
places in the code where the counters are incremented.  For example, counters
`MsgDestroy` and `MsgUnprocessedDestroy` are incremented inside the destructor
for class `TMsg`, which represents a single message:

```C++
TMsg::~TMsg() noexcept {
  assert(this);
  MsgDestroy.Increment();

  if (State != TState::Processed) {
    MsgUnprocessedDestroy.Increment();
    static TLogRateLimiter lim(std::chrono::seconds(5));

    if (lim.Test()) {
      syslog(LOG_ERR, "Possible bug: destroying unprocessed message with "
             "topic [%s] and timestamp %llu. This is expected behavior if "
             "the server is exiting due to a fatal error.", Topic.c_str(),
             static_cast<unsigned long long>(Timestamp));
      Server::BacktraceToLog();
    }
  }
}
```

### Discard Reporting

When certain problems occur, which are detailed
[here](https://github.com/tagged/bruce/blob/master/doc/design.md), Bruce will
discard messages.  When discards occur, they are tracked and reported through
Bruce's discard reporting web interface.  If you click on `get discard info`
in Bruce's web interface shown near the top of this page (i.e. send an HTTP GET
to `http://example:9090/sys/discards`), you will get a discard report that
looks something like this:

```
pid: 5843
now: 1408659550 Thu Aug 21 15:19:10 2014
version: 1.0.6.70.ga324763
report interval in seconds: 600

current (unfinished) reporting period:
    report ID: 125
    start time: 1408659029 Thu Aug 21 15:10:29 2014
    malformed msg count: 0
    unsupported API key msg count: 0
    unsupported version msg count: 0
    bad topic msg count: 0


latest finished reporting period:
    report ID: 124
    start time: 1408658429 Thu Aug 21 15:00:29 2014
    malformed msg count: 0
    unsupported API key msg count: 0
    unsupported version msg count: 0
    bad topic msg count: 0
```

The above output shows the typical case where no discards are occurring.  A
case in which discards are occurring might look something like this:

```
pid: 17706
now: 1408661249 Thu Aug 21 15:47:29 2014
version: 1.0.6.70.ga324763
report interval in seconds: 600

current (unfinished) reporting period:
    report ID: 0
    start time: 1408661147 Thu Aug 21 15:45:47 2014
    malformed msg count: 0
    unsupported API key msg count: 0
    unsupported version msg count: 0
    bad topic msg count: 15

    recent bad topic: 11[bad_topic_2]
    recent bad topic: 11[bad_topic_1]

    discard topic: 6[topic1] begin [1408661193503] end [1408661202191] count 34176
    discard topic: 6[topic2] begin [1408661210436] end [1408661219378] count 32149
```

In the above example, we see that 34176 messages were discarded for valid topic
`topic1` and 32149 messages were discarded for valid topic `topic2`.  Of the
discards for `topic1`, the earliest timestamp was 1408661193503 and the latest
was 1408661202191.  Likewise, the earliest and latest timestamps of discarded
messages for `topic2` are 1408661210436 and 1408661219378.  The timestamps are
the client-provided ones documented
[here](https://github.com/tagged/bruce/blob/master/doc/sending_messages.md#message-formats),
and are interpreted as milliseconds since the epoch.  A total of 15 messages
with invalid topics were received, and recently received invalid topics are
`bad_topic_1` and `bad_topic_2`.  Bruce's process ID is 17706, and the time
when the discard report was created is 1408661249 (represented in seconds, not
milliseconds, since the epoch).  The version of Bruce that produced the report
is `1.0.6.70.ga324763`.  The default discard report interval, as shown above,
is 600 seconds, and is configurable, as documented
[here](https://github.com/tagged/bruce/blob/master/doc/detailed_config.md).
An example Nagios script `src/bruce/scripts/bruce_discards.py` requests discard
reports from Bruce, analyzes and stores them in an Oracle database, and reports
problems.

### Queued Message Information

If you click on `get msg stats` in Bruce's web interface shown near the top of
this page (i.e. send an HTTP GET to `http://example:9090/sys/msg_stats`), you
will get queued message information that looks something like this:

```
pid: 18592
now: 1408667163 Thu Aug 21 17:26:03 2014
version: 1.0.6.70.ga324763

queued:        147  send_wait:          0  ack_wait:        147  topic: [topic1]

       147 total queued (send_wait + ack_wait)
         0 total send_wait
       147 total ack_wait
     21653 total new
     21800 total (all states: new + send_wait + ack_wait)
```

As with discard reports, you can see the process ID, current time, and Bruce's
version at the top.  It also shows that for topic `topic1`, 147 total messages
have been assigned to a broker and are being sent.  Of those 147 messages, 0
are waiting to be sent to a Kafka broker and 147 are waiting for an
acknowledgement (ACK) from a broker.  Additionally, 21653 messages are either
being batched or are new messages that Bruce has not yet started processing.
As future work, the intent is to have this last value broken into two separate
values: a count of messages being batched, and a count of messages that Bruce
has not yet started processing.

### Metadata Fetch Time

If you click on `get metadata fetch time` in Bruce's web interface shown near
the top of this page (i.e. send an HTTP GET to
`http://example:9090/sys/metadata_fetch_time`), you will get metadata fetch
time information that looks something like this:

```
pid: 18592
version: 1.0.6.70.ga324763
now (milliseconds since epoch): 1408668040576 Thu Aug 21 17:40:40 2014
metadata last updated at (milliseconds since epoch): 1408667094030 Thu Aug 21 17:24:54 2014
metadata last modified at (milliseconds since epoch): 1408667094030 Thu Aug 21 17:24:54 2014
```

The *last updated at* value indicates the last time when the metadata was
updated, but not necessarily modified.  If Bruce requests metadata, and finds
the new metadata to be identical to what it currently has, it treats this event
as a metadata update without modification.  The *last modified at* value
indicates the last time when Bruce actually replaced its metadata due to
changed information.  If the metadata has never changed since Bruce started
running, then the *last modified at* value indicates the time when Bruce
initialized its metadata during startup.

### Metadata Updates

Bruce refreshes its metadata at regular intervals.  The interval length
defaults to 15 minutes plus or minus some randomness, which is added so that
different Bruce instances will tend to spread out their requests and not all
ask for new metadata at the same time.  Configuration of the interval length is
documented
[here](https://github.com/tagged/bruce/blob/master/doc/detailed_config.md).
Additionally, you can manually cause Bruce to update its metadata.  Clicking
on the `update metadata` button in Bruce's web interface shown near the top of
this page (i.e. sending an HTTP POST to
`http://example:9090/sys/metadata_update`) causes Bruce to update its metadata.
Certain error conditions can also cause Bruce to update its metadata, as
described [here](https://github.com/tagged/bruce/blob/master/doc/design.md).

### Future work

At some point, it would be nice to replace the output format of Bruce's web
interface with JSON, so output can be parsed with off-the-shelf JSON libraries.

At this point it is helpful to have some information on
[Bruce's design](https://github.com/tagged/bruce#design-overview).