# Measurements

## Testing

### TCP
Testing the TCP performance was done with running the `download-client` program on the RPi and `download-server` on a different machine. The IP used to connect should be changed inside the client code. There are also parameters at the top of the file that can be changed.

## Nginx
Nginx testing was done using [WRK](https://github.com/wg/wrk) where Ngix would server the `test-files` folder. So you can test the performance with different file sizes. The name of the file in the `test-files` corresponds to the size of the file contents in bytes.

## Results

The `results` folder contains the data resulting from the measurements we performed. The Linux and Unikraft results are in their respective sub-folders and the main folder contains scripts to generate the graphs. `nginx.py` and `tcp.py` we used for the final graphs included in the thesis.

`tcp-100` contains the TCP transfer test results with the following parameters set that the server file:
```
#define BUFFER_SIZE 1000000
#define TIMES_TO_DOWNLOAD_BUFFER 10
#define REPEAT_TEST 100
```

`tcp-50` contains the TCP transfer test results with the following parameters set that the server file:
```
#define BUFFER_SIZE 1000000
#define TIMES_TO_DOWNLOAD_BUFFER 100
#define REPEAT_TEST 50
```

`wrk-1.txt` contains WRK testing with 1 concurrent connection using the following command:
```
wrk -c 1 -d 5m -t 1 --latency http://192.168.2.5/test-files/<FILESIZE>.txt
```

`wrk-10.txt` contains WRK testing with 10 concurrent connections using the following command:
```
wrk -c 10 -d 5m -t 1 --latency http://192.168.2.5/test-files/<FILESIZE>.txt
```

The TCP test program for Unikraft was based on [Unikraft's Hello World program](https://github.com/unikraft/app-helloworld). And the Nginx testing on [Unikraft's Nginx program](https://github.com/unikraft/app-nginx).

The exact configuration used can be found in the `configs` folder and each file can be copied over the a correctly structured project (like written in the main README.md file). Although, at least the following values would need to be updated:
```
CONFIG_UK_BASE
CONFIG_UK_APP
CONFIG_LIBVFSCORE_AUTOMOUNT_EINITRD_PATH
```
