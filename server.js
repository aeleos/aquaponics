var express = require('express');
var app = express();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var path = require('path');
var spawn = require('child_process').spawn;
var fs = require('fs');
var sp = require('serialport');
var SerialPort = require("serialport").SerialPort
var serialPort = new SerialPort('/dev/ttyACM0', {
  baudRate: 9600,
  parser: sp.parsers.readline("\n")
});
var proc;
var humidity; //1
var airtemp; //2
var watertemp; //3
var ph; //4
var moisture; //5

var sockets = {};

serialPort.on('data', onData);

function onData(data){
  if (data.charAt(0) == 1){
    console.log("DATA RECOGNIZED AS: humidity");
    console.log(data.substr(2, data.length));
	var humidity = data.substr(2, data.length)
	
  } else if (data.charAt(0) == 2){
    console.log("DATA RECOGNIZED AS: airtemp");
    console.log(data.substr(2, data.length));
	var airtemp = data.substr(2, data.length)
	
  } else if (data.charAt(0) == 3){
    console.log("DATA RECOGNIZED AS: watertemp");
    console.log(data.substr(2, data.length));
	var watertemp = data.substr(2, data.length)
	
  } else if (data.charAt(0) == 4){
    console.log("DATA RECOGNIZED AS: ph");
    console.log(data.substr(2, data.length));
	var ph = data.substr(2, data.length)

  } else if (data.charAt(0) == 5){
    console.log("DATA RECOGNIZED AS: moisture");
    console.log(data.substr(2, data.length));
	var moisture = data.substr(2, data.length)

  } else {
    console.log("ERROR: DATA INPUT NOT RECOGNIZED");
  }
}

io.on('connection', function(socket){
  sockets[socket.id] = socket;
  console.log("Total clients connected : ", Object.keys(sockets).length);
 
  socket.on('disconnect', function() {
    delete sockets[socket.id];
 
    // no more sockets, kill the stream
    if (Object.keys(sockets).length == 0) {
      app.set('watchingFile', false);
      if (proc) proc.kill();
      fs.unwatchFile('./stream/image_stream.jpg');
    }
  });
 
  socket.on('start-stream', function() {
    startStreaming(io);
  });


});

app.use(express.static(path.join(__dirname, 'public')));

app.use('/', express.static(path.join(__dirname, 'stream')));

app.get('/', function(req, res){
  res.sendFile(__dirname + '/index.html');
});

http.listen(80, function(){
  console.log('listening on *:80');
});

function stopStreaming() {
  if (Object.keys(sockets).length == 0) {
    app.set('watchingFile', false);
    if (proc) proc.kill();
    fs.unwatchFile('./stream/image_stream.jpg');
  }
}

function startStreaming(io) {
 
  if (app.get('watchingFile')) {
    io.sockets.emit('liveStream', 'image_stream.jpg?_t=' + (Math.random() * 100000));
    return;
  }
 
  var args = ["-w", "640", "-h", "480", "-o", "./stream/image_stream.jpg", "-t", "999999999", "-tl", "100"];
  proc = spawn('raspistill', args);
 
  console.log('Watching for changes...');
 
  app.set('watchingFile', true);
 
  fs.watchFile('./stream/image_stream.jpg', function(current, previous) {
    io.sockets.emit('liveStream', 'image_stream.jpg?_t=' + (Math.random() * 100000));
  })
 
}
