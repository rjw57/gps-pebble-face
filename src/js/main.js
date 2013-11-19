/* This file is concatenated with .js files in lib/ to form pebble-js-app.js */

Pebble.addEventListener("ready",
  function(e) {
    console.log("Starting JavaScript app");

    // WGS84 lat/lng
    var srcProj = proj4.Proj('EPSG:4326');
    
    // British National Grid
    var dstProj = proj4.Proj('+proj=tmerc +lat_0=49 +lon_0=-2 +k=0.9996012717 +x_0=400000 +y_0=-100000 +ellps=airy +datum=OSGB36 +units=m');

    // None-null if we're still waiting for an Ack from the Pebble (or a failure). If non-null, it's a timeout.
    var waitingForAck = null;

    var pointToSquare = function(pt, bounds) {
      var relPt = { x: pt.x - bounds.origin.x, y: pt.y - bounds.origin.y };
      var sq = { x: Math.floor(relPt.x / (bounds.size.w / 5)), y: Math.floor(relPt.y / (bounds.size.h / 5)) };
      var sqIndex = ((4-sq.y) * 5) + sq.x;
      var sqLetter = "ABCDEFGHJKLMNOPQRSTUVWXYZ"[sqIndex];
      var sqBounds = {
        origin: { x: sq.x*(bounds.size.w / 5), y: sq.y*(bounds.size.h / 5) },
        size: { w: (bounds.size.w / 5), h: (bounds.size.h / 5) }
      };
      return { letter: sqLetter, bounds: sqBounds, relative: relPt };
    }

    var posSuccessFunc = function(position) {
      // Don't do anything if we're still waiting for the Pebble to acknowledge
      // our last update.
      if(waitingForAck !== null) {
        return;
      }

      console.log('Got position:' + JSON.stringify(position));

      // Is this within the bounds of the BNG?
      if((position.coords.longitude <= -7.56) || (position.coords.longitude >= 1.78) ||
            (position.coords.latitude <= 49.96) || (position.coords.latitude >= 60.84)) {
        // no, we're outside the BNG
        return;
      }

      // British National Grid is 5x5 500km x 500km squares
      // We need to add these offsets because (0,0) is actually south-westerly portion of square S
      var bngBounds = {
        origin: { x: -2*500*1000, y: -500*1000 },
        size: { w: 5*500*1000, h: 5*500*1000 },
      };
      
      // Calculate numeric BNG co-ord
      var bngCoords = proj4(srcProj, dstProj, [position.coords.longitude, position.coords.latitude]);
      console.log('BNG position:' + JSON.stringify(bngCoords));

      var largeSq = pointToSquare({x:bngCoords[0], y:bngCoords[1]}, bngBounds);
      var smallSq = pointToSquare(largeSq.relative, largeSq.bounds);
      var easting = ("00000" + Math.round(smallSq.relative.x).toString()).slice(-5);
      var northing = ("00000" + Math.round(smallSq.relative.y).toString()).slice(-5);
      var reference = largeSq.letter + smallSq.letter + easting + northing;

      var toDMS = function(v, posneg) {
          return Math.floor(v).toString() + '\u00b0' + Math.floor((v * 60) % 60).toString() + "'" + Math.floor((v * 60 * 60) % 60).toString() + '"' +
              ((v<0) ? posneg[1] : posneg[0]);
      }

      var msg = {
        bng: reference,
        latitude: toDMS(position.coords.latitude, 'NS'),
        longitude: toDMS(position.coords.longitude, 'WE'),
      };

      console.log("Sending: " + JSON.stringify(msg));

      waitingForAck = setTimeout(function() { console.log('Timed out waiting for response from Pebble.'); waitingForAck = null; }, 20*1000);

      var tid = Pebble.sendAppMessage(
        msg,
        function(e) { clearTimeout(waitingForAck); waitingForAck = null; console.log("Delivered message " + e.data.transactionId); },
        function(e) { clearTimeout(waitingForAck); waitingForAck = null; console.log("Unable to deliver message " + e.data.transactionId + ". Error: " + e.error.message); }
      );

      console.log("Queued message " + tid);
    };

    var posErrorFunc = function() {
    };

    Pebble.addEventListener("appmessage", function(e) {
        console.log('Got ping from watch: ' + JSON.stringify(e));

        // Start watching position (happy with updates every minute).
        navigator.geolocation.getCurrentPosition(posSuccessFunc, posErrorFunc, {
          enableHighAccuracy: true,
          maximumAge: 15*1000,
          timeout: 15*1000,
        });
    });

    console.log("JavaScript app ready and running!");
  }
);

// vim:sw=2:sts=2:et
