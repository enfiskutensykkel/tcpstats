tcpstats
========
Calculate various statistics for different TCP flows in a trace file.


Limitations
-----------
Since this program only looks at one (sender-side) trace, loss is determined
out of retransmissions. This might not always lead to correct calculations.

If not all segments are in the trace (i.e. the kernel dropped some), 
some calculations may be incorrect.

It also doesn't look at SACK/DSACK data, even though these might be enabled.
I'm not fully sure what the consequences of not supporting this yet might be.
