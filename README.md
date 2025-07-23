# Sistema IoT para la Monitorización de la Seguridad y Riesgos Laborales en la Industria
## INSTALACIÓN 
Instrucciones para replicar el proyecto
1. Instalar Arduino IDE
2. Navegar al directorio server
3. docker-compose up -d
4. Mover todos los directorios excepto server, buzzerLib y bot_bdLib a la carpeta donde se guardan los sketches de Arduino IDE
5. Mover los directorios buzzerLib y bot_bdLib a la carpeta "libraries" de Arduino IDE
6. En fichero bd_bot.cpp del directorio server, cambiar el valor de la variable INFLUXDB_URL a  "http://nombre.local", siendo nombre el hostname del ordenador

### Bot Telegram
1. Cambiar las variables BOTtoken y CHAT_ID del fichero bd_bot.cpp por los valores del bot
2. Para crear bot usar @BotFather en Telegram
3. El BOTtoken lo proporciona el propio BotFather
4. Crear un grupo de Telegram incluyendo el bot creado y los usuarios a los que le deben llegar las alertas
5. Enviar un mensaje al grupo
6. Hacer petición https://api.telegram.org/bot<TU_TOKEN>/getUpdates para ver el CHAT_ID
   6.1 Si no funciona, conceder al bot permisos de admin en el grupo

### WINDOWS
Instalar y ejecutar Bonojour: https://support.apple.com/es-es/106380

### LINUX
Instalar y habilitar el servicio Avahi

## EJECUCIÓN 
1. Subir el código a la placa (completo.ino)
2. Conectar el ordenador al AP generado por el ESP8266
3. Elegir red LAN (para que funcione mDNS)
4. InfluxDB: http://localhost:8086
5. Grafana: http://localhost:3000


