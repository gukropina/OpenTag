/*
Notes:
Run in dev mode with command: npm run dev   this makes node restart whenever you save the app
Start with command: npm start (in terminal), make sure you are in the "backend" module
End with command: control c (in terminal)

To install packages, run "npm install"

Set up the constants for your arduino port (change below constants if needed)

You need to have set up a mongoDb already

You are using dotenv to store environmental variables in the .env file (https://www.npmjs.com/package/dotenv)
npm install dotenv --save
add ‘.env’ to your ‘.gitignore’ file so that Git ignores it and it never ends up on GitHub. 
You can add any keys you want to this file.

Before launching to production, check MongoDB list: https://www.mongodb.com/docs/manual/administration/security-checklist/
*/
const express = require('express');
const bodyParser = require('body-parser');
const mongoose = require('mongoose');
const cors = require('cors');

//start express
const app = express();
app.use(bodyParser.json());  //use the body parser
app.use(cors());

//include environmental variables
require('dotenv').config();


//declare constants for arduino COM port
const ArduinoCOMPort = 'COM19';
const ArduinoBaud = 115200;

//global stats device id (in the mongoDB)
const GlobalStatsID = 63;

// Import dependencies
const SerialPort = require("serialport");
const Readline = require("@serialport/parser-readline");

// Defining the serial port
const port = new SerialPort(ArduinoCOMPort, {
    baudRate: ArduinoBaud,
});

main().catch(err => console.log(err));

async function main() {
  await mongoose.connect( process.env.MONGO_DB_URL_PROD );
  // before launching to production, check https://www.mongodb.com/docs/manual/administration/security-checklist/
}

//set up the event schema
const eventSchema = new mongoose.Schema({
    event_name: { type: String, required: true },
    tag_name: { type: String, required: true },
    team_name: { type: String, required: true },
    event_num: { type: Number, required: true },
    who_was_hit: { type: Number, required: true },
    hit_by: { type: Number, required: false },
    tag_received: { type: Number, required: false },
    tag: { type: Number, required: false },
    team: { type: Number, required: false },
    bonus_damage: { type: Number, required: false },
    checksum: { type: Number, required: false },
    checksum_matches: { type: String, required: true }
},
{
    timestamps: true,      //enable mongoDb timestamps to automatically be applied (created and update)
    collection: 'events'   //select collection to save events to
} );

//create a new module to use to create new events
const Event = mongoose.model('Event', eventSchema);

const overallstatsSchema = new mongoose.Schema({
    device_id: { type: Number, required: true },
    alive_or_dead: { type: String, required: true },
    kills: { type: Number, required: true },
    deaths: { type: Number, required: true },
    shots: { type: Number, required: true },
    hits: { type: Number, required: true },
    game: { type: Number, required: true },
    name: { type: String, required: true },
    survival_mode: { type: String, required: true },
    auto_revive: { type: String, required: true },
    use_ammo: { type: String, required: true },
    auto_aim_assist: { type: String, required: true },
    juggernaut: { type: String, required: true },
    playing: { type: String, required: true }
},
{
    timestamps: true,
    collection: 'overall-stats'
});

//create a new module to use to look up overall stats
const Overall_Stats = mongoose.model('overall-stats', overallstatsSchema);


var msg = 'Hello World';
console.log(msg);

//setup Express app

app.get('/stats', async (req, res) => {
 const all = await Overall_Stats.find(); //finds all stats
 res.send(all); //send it!
});

app.get('/stats/:id', async (req, res) => {
    //update the name for that player
    let ret = await Overall_Stats.findById(req.params.id);
    res.send(ret);
});

app.post('/stats/:id', async (req, res) => {
    //update the name for that player
    let ret = await Overall_Stats.findByIdAndUpdate(req.params.id, {"name": req.body.name}, {
        new: true
    });
    res.send(ret);
});

app.get('/events', async (req, res) => {
    //get the last 50 events
    let ret = await Event.find({}, null, {
        limit: 50,
        sort: { createdAt: -1 }
    });
    res.send(ret);
});

app.get('/events/who_was_hit/:device_id', async (req, res) => {
    //get the last 50 events
    let ret = await Event.find({"who_was_hit": req.params.device_id}, null, {
        limit: 50,
        sort: { createdAt: -1 }
    });
    res.send(ret);
});

app.get('/events/hit_by/:device_id', async (req, res) => {
    //get the last 50 events
    let ret = await Event.find({"hit_by": req.params.device_id}, null, {
        limit: 50,
        sort: { createdAt: -1 }
    });
    res.send(ret);
});

app.post('/command/:id', async (req, res) => {
    //send a command to a specific player
    let Command_To_Send = '11,' + req.params.id.toString() + ',' + req.body.command.toString() + '\n';
    port.write(Command_To_Send, (err) => {
        if (err) {
          return console.log('Error on write: ', err.message);
        }
        //console.log('message written');
      });
    console.log("Sent Command: " + Command_To_Send);
    res.send(req.params.id);
});

app.listen(4001, () => {
    console.log('Listening on 4001');
});


// The Serial port parser
const parser = new Readline({ delimiter: '\n' });  //create a new parser that checks for end of lines
port.pipe(parser);                                 //attach parser to the serial port

// Open the port
port.on("open", () => {
    console.log('serial port open');
  });

  //read the port data and do something with it
  parser.on('data', data =>{
    var array = JSON.parse("[" + data + "]");   //parse the data into an array
    //determine the type of event's name
    var Event_Name = "Unknown";
    switch (array[0]) {
    case 1:
        Event_Name = "hit";
        break;
    case 2:
        Event_Name = "hit_armor";
        break;
    case 3:
        Event_Name = "tagged_out";
        break;
    case 4:
        Event_Name = "tagged_juggernaut_out";
        break;
    case 5:
        Event_Name = "device_started";
        break;
    case 6:
        Event_Name = "revived";
        break;  
    case 7:
        Event_Name = "juggernaut_spawn";
        break;
    case 8:
        Event_Name = "automatic_aim_assist";
        break;
    case 9:
        Event_Name = "manual_aim_assist";
        break;
    case 10:
        Event_Name = "invalid_tag_received";
        break; 
    case 11:
        Event_Name = "update_game";
        break;
    case 12:
        Event_Name = "update_game_ack";
        break;    
    default:
        console.log(`Event not known`);
    }
    
    //determine the name of the tag
    var Tag_Name = "Unknown";
    if(array[0] != 12){
        switch ((array[3] >> 19)) {
            case 0:
                Tag_Name = "Double Damage Tag";
                break;
            case 1:
                Tag_Name = "Normal Tag";
                break;
            case 2:
                Tag_Name = "Heal";
                break;
            case 3:
                Tag_Name = "Revive";
                break;
            case 4:
                Tag_Name = "Fire";
                break;
            case 5:
                Tag_Name = "Ice";
                break;
            case 6:
                Tag_Name = "Poison";
                break;  
            case 7:
                Tag_Name = "Rock";
                break;
            case 8:
                Tag_Name = "Water";
                break;
            case 9:
                Tag_Name = "Lightning";
                break;
            case 10:
                Tag_Name = "invalid tag received";
                break; 
            case 11:
                Tag_Name = "Triple Damage Tag";
                break; 
            case 12:
                Tag_Name = "Wind";
                break;
            case 13:
                Tag_Name = "Explosion";
                break;
            case 17:
                Tag_Name = "Auto-Revive Mode On/Off";
                break;
            case 18:
                Tag_Name = "Survival Mode On/Off";
                break;
            case 19:
                Tag_Name = "Aim Assist On/Off";
                break;
            case 20:
                Tag_Name = "Ammo Mode On/Off";
                break;
            case 21:
                Tag_Name = "Manual Aim Detection";
                break;
            case 22:
                Tag_Name = "Automatic Aim Detection";
                break;
            case 23:
                Tag_Name = "King of the Hill Mode";
                break;
            case 24:
                Tag_Name = "Change Class Rune";
                break;  
            case 25:
                Tag_Name = "Open Rune (not programmed)";
                break;
            case 26:
                Tag_Name = "Ammo Refill Rune";
                break;
            case 27:
                Tag_Name = "Extra Life Rune";
                break;
            case 28:
                Tag_Name = "Armor Rune";
                break; 
            case 29:
                Tag_Name = "Arcane Rune";
                break; 
            case 30:
                Tag_Name = "Health Rune";
                break;
            case 31:
                Tag_Name = "Damage Rune";
                break;
            default:
                console.log(`Tag not known`);
        }
    }
    else{
        switch ((array[2])) {
            //if you receive an acknowledgement, note what type it is
            case 1:
                Tag_Name = "ACK Survival Mode On";
                break;
            case 2:
                Tag_Name = "ACK Survival Mode Off";
                break;
            case 3:
                Tag_Name = "Ack Auto Revive On";
                break;
            case 4:
                Tag_Name = "Ack Auto Revive Off";
                break;
            case 5:
                Tag_Name = "Ack Ammo use On";
                break;
            case 6:
                Tag_Name = "Ack Ammo use Off";
                break;  
            case 7:
                Tag_Name = "Ack Aim Assist On";
                break;
            case 8:
                Tag_Name = "Ack Aim Assist Off";
                break;
            case 9:
                Tag_Name = "Ack Double Damage Rune Received";
                break;
            case 10:
                Tag_Name = "Ack Double Health Rune Received";
                break; 
            case 11:
                Tag_Name = "Ack Arcane Rune Received";
                break; 
            case 12:
                Tag_Name = "Ack Double Armor Rune Received";
                break;
            case 13:
                Tag_Name = "Ack Extra Life Rune Received";
                break;
            case 14:
                Tag_Name = "Ack Ammo Rune Received";
                break;
            default:
                Tag_Name = "Unknown Ack Received";
        }
    }
    
    
    //calculate team name
    var Team_Name = "Unknown";
    switch ( ((array[3] >> 16) & 0b111) ) {
    case 0:
        Team_Name = "White";
        break;
    case 1:
        Team_Name = "Red";
        break;
    case 2:
        Team_Name = "Blue";
        break;
    case 3:
        Team_Name = "Green";
        break;
    case 4:
        Team_Name = "Orange";
        break;
    case 5:
        Team_Name = "Magenta";
        break;
    case 6:
        Team_Name = "Yellow";
        break;  
    case 7:
        Team_Name = "Purple";
        break;
    default:
        console.log(`Team not known`);
    }
    //create a new document in mongo db, using the module, based on a schema https://mongoosejs.com/docs/models.html
    /*
    Serial.println(F("Checksum matched"));
          Serial.print(F("Tag: "));
          Serial.println((message_array[0] >> 3)); 
          Serial.print(F("Team: "));
          Serial.println((message_array[0] & 0x07));
          Serial.print(F("Bonus Damage: "));
          Serial.println((((message_array[1] & B10000000) >> 3) + ((message_array[2] & 0xF0) >> 4)));
          Serial.print(F("Device ID: "));
          Serial.println((message_array[1] & B01111111));
          Serial.print(F("Checksum: "));
          Serial.println((message_array[2] & 0x0F));
    */
   //calculate the checksum on the tag
    var Calculated_Checksum_1 = (array[3] >> 20) + ((array[3] >> 16) & 0b1111) + ((array[3] >> 12) & 0b1111) + 
                             ((array[3] >> 8) & 0b1111) + ((array[3] >> 4) & 0b1111);
    var Calculated_Checksum = (Calculated_Checksum_1 & 0b1111) + (Calculated_Checksum_1 >> 4);
    var Checksum_Matched = "No"
    if(Calculated_Checksum == (array[3] & 0b1111)){
        Checksum_Matched = "Yes";
    }
    const new_event = new Event ({
        event_num: array[0],
        event_name: Event_Name,
        who_was_hit: array[1],
        hit_by: array[2],
        tag_received: array[3],
        tag: (array[3] >> 19),
        team: ((array[3] >> 16) & 0b111),
        bonus_damage: (((array[3] &0b1000000000000000) >> 11) + ((array[3] >> 4) & 0b1111) ),
        checksum: (array[3] & 0b1111),
        checksum_matches: Checksum_Matched,
        tag_name: Tag_Name,
        team_name: Team_Name
    });
    //save document, and if there is an error, return error
    new_event.save(function (err) {
        if (err) console.log(err);
        // saved!
      });
    
    //print data
    
    console.log('event_num', array[0]);
    console.log('who_was_hit', array[1]);
    console.log('hit_by', array[2]);
    console.log('tag_received', array[3]);
    console.log('event_name:', Event_Name);
    

    //now, update the device's total stats based on what happened
    switch (array[0]) {
        case 1:
            //update the "hits" on the device that hit someone
            var query = {'device_id': array[2]};  //set up the query to find the right device
            //for finde one and update, see: https://mongoosejs.com/docs/api.html#model_Model.findOneAndUpdate
            //for incrementing a variable up, see https://docs.mongodb.com/manual/reference/operator/update/inc/
            Overall_Stats.findOneAndUpdate(query, {$inc : {'hits' : 1}}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });

            //update global stats too
            query = {'device_id': GlobalStatsID};
            Overall_Stats.findOneAndUpdate(query, {$inc : {'hits' : 1}}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });
            break;
        case 2:
            //update the "hits" on the device that hit someone (hit armor)
            var query = {'device_id': array[2]};  //set up the query to find the right device
            //for finde one and update, see: https://mongoosejs.com/docs/api.html#model_Model.findOneAndUpdate
            //for incrementing a variable up, see https://docs.mongodb.com/manual/reference/operator/update/inc/
            Overall_Stats.findOneAndUpdate(query, {$inc : {'hits' : 1}}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });

            //update global stats too
            query = {'device_id': GlobalStatsID};
            Overall_Stats.findOneAndUpdate(query, {$inc : {'hits' : 1}}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });
            break;
        case 3:
            //update the "hits" on the device that hit someone
            var query = {'device_id': array[2]};  //set up the query to find the right device
            //for finde one and update, see: https://mongoosejs.com/docs/api.html#model_Model.findOneAndUpdate
            //for incrementing a variable up, see https://docs.mongodb.com/manual/reference/operator/update/inc/
            Overall_Stats.findOneAndUpdate(query, {$inc : {'hits' : 1}, $inc : {'kills' : 1}}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });

            //update the person who is out to "out"
            var query = {'device_id': array[1]};  //set up the query to find the right device
            Overall_Stats.findOneAndUpdate(query, {'alive_or_dead': "dead", $inc : {'deaths' : 1}}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });

            //update global stats too
            query = {'device_id': GlobalStatsID};
            Overall_Stats.findOneAndUpdate(query, {'alive_or_dead': "dead", $inc : {'deaths' : 1, 'kills': 1}}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });
            break;
        case 4:
            //update the "hits" on the device that hit someone
            var query = {'device_id': array[2]};  //set up the query to find the right device
            //for finde one and update, see: https://mongoosejs.com/docs/api.html#model_Model.findOneAndUpdate
            //for incrementing a variable up, see https://docs.mongodb.com/manual/reference/operator/update/inc/
            Overall_Stats.findOneAndUpdate(query, {$inc : {'hits' : 1}, $inc : {'kills' : 1}}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });

            //update the person who is out to "out"
            var query = {'device_id': array[1]};  //set up the query to find the right device
            Overall_Stats.findOneAndUpdate(query, {'alive_or_dead': "dead", $inc : {'deaths' : 1}}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });

            query = {'device_id': GlobalStatsID};
            Overall_Stats.findOneAndUpdate(query, {$inc : {'hits' : 1}, $inc : {'kills' : 1, 'deaths' : 1}}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });
            break;
        case 5:
            //update the person who is now alive to "alive"
            var query = {'device_id': array[1]};  //set up the query to find the right device
            Overall_Stats.findOneAndUpdate(query, {'alive_or_dead': "alive", 'playing': "yes"}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });
            break;
        case 6:
            //update the person who is now alive to "alive"
            var query = {'device_id': array[1]};  //set up the query to find the right device
            Overall_Stats.findOneAndUpdate(query, {'alive_or_dead': "alive"}, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
                //return console.log('Succesfully saved.');
            });
            break;  
        case 7:
            //juggernaut entered the playing field
            
            break;
        case 8:
            
            break;
        case 9:
            
            break;
        case 10:
            
            break;
        case 11:
        
            break;
        case 12:
            //in this case, you are receiving an acknowledgement of a change of game state. Log it in mongodb
            var query = {'device_id': array[1]};  //set up the query to find the right device
            var result_to_update = {};
            switch ((array[2])) {
                //if you receive an acknowledgement, note what type it is
                case 1:
                    result_to_update = {"survival_mode": "On"};
                    break;
                case 2:
                    result_to_update = {"survival_mode": "Off"};
                    break;
                case 3:
                    result_to_update = {"auto_revive": "On"};
                    break;
                case 4:
                    result_to_update = {"auto_revive": "Off"};
                    break;
                case 5:
                    result_to_update = {"use_ammo": "On"};
                    break;
                case 6:
                    result_to_update = {"use_ammo": "Off"};
                    break;  
                case 7:
                    result_to_update = {"auto_aim_assist": "On"};
                    break;
                case 8:
                    result_to_update = {"auto_aim_assist": "Off"};
                    break;
                default:
                    result_to_update = {};
            }
            Overall_Stats.findOneAndUpdate(query, result_to_update, {upsert: true}, function(err, doc) {
                if (err) return console.log('Error: ', err);
            });
            break;    
        default:
            console.log(`Stats Not Updated`);
        }
  });