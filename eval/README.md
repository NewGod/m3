# m3-eval

Scripts for evaluating performance of M3.

## mperf test

- Run `init` to create virtual network
- Start `mperf-server` on default namespace, bind ip to `10.100.4.1` and port `23334` (execute `mperf-server -s 10.100.4.1 -p 23334`)
- Run `./m3`
- Logs of proxy and relay are stored in directory `m3/temp`