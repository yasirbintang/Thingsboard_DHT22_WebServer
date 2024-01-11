**Project Thingsboard DHT22 + Webserver**

Bagian dari project Monitoring Kondisi Ruang Server berbasis ESP32 + Thingsboard, project ini berfungsi untuk monitoring kondisi ruang server dari sisi temperatur + kelembapan pada bagian perangkat keras yang berada di dalam ruang server seperti PC Server, NAS, Penyimpanan data di server, dll.

Progress:
1. Sudah dapat menampilkan data dari DHT Sensor secara realtime pada halaman Web Server ESP32
2. Penyimpanan value variable inputan web server di simpan dalam file-file txt yang terpisah. (misal : ssid di simpan di dalam ssid.txt, dst)
3. Halaman web server sudah include di dalam ESP32 (diupload melalui uploader ESP32 Arduino IDE)

Next Task :
1. Tampilkan data DHT22 ke dalam Thingboard Localserver dan Web Server
2. Simpan value variable web server ke dalam 1 file JSON yang berisi (SSID, Pass, IP, Gateway, TBServer, TBToken)
3. Inovasi halaman Web server yang lebih user friendly.

 
