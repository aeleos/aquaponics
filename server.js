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
var dl = require('delivery');
var delivery = dl.listen(io);
var proc;
var humidity; //1
var airtemp; //2
var watertemp; //3
var ph; //4
var moisture; //5

var sockets = {};

var cameraOptions = {
    width       : 640,
    height      : 480,
    mode        : "photo",
    awb         : 'cloud',
    output      : 'public/image_stream.jpg',
    q           : 50,
    //rot         : 270,
    nopreview   : true,
    timeout     : 0,
    timelapse   : 500,
};

var camera = new require("raspicam")(cameraOptions);
// camera.start();

serialPort.on('data', onData);

function onData(data){
  if (data.charAt(0) == 1){
    // console.log("DATA RECOGNIZED AS: humidity");
	humidity = data.substr(2, data.length);
	// console.log(humidity);

  } else if (data.charAt(0) == 2){
    // console.log("DATA RECOGNIZED AS: airtemp");
	airtemp = data.substr(2, data.length);
	// console.log(airtemp);

  } else if (data.charAt(0) == 3){
    // console.log("DATA RECOGNIZED AS: watertemp");
	watertemp = data.substr(2, data.length);
	// console.log(watertemp);
	
  } else if (data.charAt(0) == 4){
    // console.log("DATA RECOGNIZED AS: ph");
	ph = data.substr(2, data.length);
	// console.log(ph);

  } else if (data.charAt(0) == 5){
    // console.log("DATA RECOGNIZED AS: moisture");
	moisture = data.substr(2, data.length);
	// console.log(moisture);
  } else {
    console.log("ERROR: DATA INPUT NOT RECOGNIZED");
  }
  io.sockets.emit('humidity', { humidity: humidity });
  io.sockets.emit('airtemp', { airtemp: airtemp });
  io.sockets.emit('watertemp', { watertemp: watertemp });
  io.sockets.emit('ph', { ph: ph });
  io.sockets.emit('moisture', { moisture: moisture });

}

io.on('connection', function(socket){
  sockets[socket.id] = socket;
  console.log("Total clients connected : ", Object.keys(sockets).length);
 
  socket.on('disconnect', function() {
    delete sockets[socket.id];
 
    // no more sockets, kill the stream
    if (Object.keys(sockets).length == 0) {
		camera.stop();
    }
  });

  
  socket.on('start-datastream', function() {
	  console.log(humidity);
  });
   socket.on('start-stream', function() {
    startStreaming(io);
  });
	

});

app.use(express.static(path.join(__dirname, 'public')));

app.use('/', express.static(path.join(__dirname, 'stream')));

// app.use('/', express.static(path.join(__dirname, 'images')));

app.get('/', function(req, res){
  res.sendFile(__dirname + '/index.html');
});



http.listen(80, function(){
  console.log('listening on *:80');
});


// camera.on("read", function(err, timestamp, filename){ 
    // delivery.send({
      // name: '-camera.jpg',
      // path : './images/camera.jpg'
    // });

// });



// camera.on("exit", function()
// {
    // camera.stop();
    // console.log('Restarting camera...')
    // camera.start()
// });

function stopStreaming() {
  if (Object.keys(sockets).length == 0) {
    // app.set('watchingFile', false);
    // if (proc) proc.kill();
    // fs.unwatchFile('./public/image_stream.jpg');
	camera.stop();
  }
}
 
function startStreaming(io) {
 
  if (!camera.start()) {
    io.sockets.emit('liveStream', 'image_stream.jpg?_t=' + (Math.random() * 100000));
    return;
  }
 
  // var args = ["-w", "640", "-h", "480", "-o", "./public/image_stream.jpg", "-t", "999999999", "-tl", "100"];
  // proc = spawn('raspistill', args);
  camera.start();
 
  console.log('Watching for changes...');
 

 
}

camera.on("read", function(err, timestamp, filename){ 
    io.sockets.emit('liveStream', 'image_stream.jpg?_t=' + (Math.random() * 100000));
});




