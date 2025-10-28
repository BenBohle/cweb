
benbo@BohleDev:/mnt/c/Users/benbo/wrk$ wrk -t4 -c100 -d10s http://127.0.0.1:8080/home
Running 10s test @ http://127.0.0.1:8080/home
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     2.88ms  454.56us   8.10ms   83.20%
    Req/Sec     8.71k     1.05k   10.51k    73.27%
  350145 requests in 10.10s, 1.80GB read
Requests/sec:  34654.91
Transfer/sec:    182.73MB


benbo@BohleDev:/mnt/c/Users/benbo/wrk$ wrk -t4 -c1000 -d10s http://127.0.0.1:8080/
home
Running 10s test @ http://127.0.0.1:8080/home
  4 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    29.42ms    4.65ms  73.40ms   95.52%
    Req/Sec     8.55k     1.06k   10.04k    83.00%
  340420 requests in 10.05s, 1.75GB read
Requests/sec:  33880.10
Transfer/sec:    178.65MB