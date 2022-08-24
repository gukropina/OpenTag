import React, {useState, useEffect } from 'react';
//import axios from 'axios';
import IndividualMessage from './IndividualMessage';
import * as Realm from 'realm-web';

export default () => {
    const [stats, setStats] = useState([]);

    let interval;     //this is the function to turn on/off the 
    let counter = 0;


    //get list of all stats
    const fetchStats = async () => {
        //now, check to see if you should stop automatically calling the function automatically
        
        //get commands values locally
        //const res = await axios.get(process.env.REACT_APP_BACKEND_URL + 'stats');
        //setStats(res.data);

        //use Realm to get data from Realm
        //environmental variables aren't working for some reason: process.env.REALM_APP_ID
        const app = new Realm.App({id: 'laser-tag-1-wmptj'});
        const credentials = Realm.Credentials.anonymous();
        const user = await app.logIn(credentials);
        const current_stats = await user.functions.GetAllStats();
        setStats(current_stats);
        
        //only update the page for 10 minutes
        counter++;
        if(counter > 120){
            clearTimeout(interval);
            console.log('Auto-Refresh off, as refresh count is: %d', counter);
        }
    };


    useEffect(() => {
        fetchStats();
        interval = setInterval(() => {
            fetchStats();
          }, 5000);         //fetch data from the back end every 5000 ms
          return () => clearInterval(interval);
    }, []);

    // react colors: https://reactnative.dev/docs/colors

    const renderedStats = stats.map(stats => {
        return <div 
            className="card" 
            style={{ width: '30%', marginBottom: '20px' }}
            key={stats._id}
            >
                <div className="card-body">
                    <h3>{stats.name}</h3>
                    <ul>
                        <li>Device Id: {stats.device_id}</li>
                        <li style={{backgroundColor: stats.survival_mode === "On" ? "lightgreen" : ""}}>Survival Mode (1-2): {stats.survival_mode}</li>
                        <li style={{backgroundColor: stats.auto_revive === "On" ? "lightgreen" : ""}}>Auto-revive Mode (3-4): {stats.auto_revive}</li>
                        <li style={{backgroundColor: stats.use_ammo === "On" ? "lightgreen" : ""}}>Use Ammo Mode (5-6): {stats.use_ammo}</li>
                        <li style={{backgroundColor: stats.auto_aim_assist === "On" ? "lightgreen" : ""}}>Auto Aim Assist Mode (7-8): {stats.auto_aim_assist}</li>
                        <li style={{backgroundColor: stats.juggernaut === "On" ? "turquoise" : ""}}>Juggernaut: {stats.juggernaut}</li>      
                    </ul>
                    <IndividualMessage deviceID={stats.device_id}/>
                </div>
        </div>
    });

    return <div className="d-flex flex-row flex-wrap justify-content-between">
        {renderedStats}
    </div>;
}