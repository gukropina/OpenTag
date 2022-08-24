import React, {useState, useEffect } from 'react';
//import axios from 'axios';
import ChangeName from './ChangeName';
import * as Realm from 'realm-web';

export default () => {
    const [stats, setStats] = useState([]);

    //I only want to have the website fetch data automatically for about an hour
    let interval;     //this is the function to turn on/off the 
    let counter = 0;

    //get list of all stats
    const fetchStats = async () => {
        //use axios to get data from local host
        //const res = await axios.get(process.env.REACT_APP_BACKEND_URL + 'stats');
        //setStats(res.data);

        //use Realm to get data from Realm
        //environmental variables aren't working for some reason: process.env.REALM_APP_ID
        const app = new Realm.App({id: 'laser-tag-1-wmptj'});
        const credentials = Realm.Credentials.anonymous();
        const user = await app.logIn(credentials);
        const current_stats = await user.functions.GetAllStats();

        //now, check to see if you should stop automatically calling the function automatically
        counter++;
        if(counter > 120){
            clearTimeout(interval);
            console.log('Auto-Refresh off, as refresh count is: %d', counter);
        }


        //now, update the stats on the page
        setStats(current_stats);
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
            style={{ width: '50%', marginBottom: '20px' }}
            key={stats._id}
            >
                <div className="card-body">
                    <h3>{stats.name}</h3>
                    <ul>
                        <li style={{backgroundColor: stats.alive_or_dead === "alive" ? "lightgreen" : "lightpink"}}>Status: {stats.alive_or_dead}</li>
                        <li>{stats.kills} Kills</li>
                        <li>{stats.deaths} Deaths</li>
                        <li>{stats.shots} Shots</li>
                        <li>{stats.hits} Hits</li> 
                        <hr></hr>
                        <li>Device Id: {stats.device_id}</li>
                        <li>Game: {stats.game}</li>
                    </ul>
                    <ChangeName statsId={stats._id}/>
                </div>
        </div>
    });

    return <div className="d-flex flex-row flex-wrap justify-content-between">
        {renderedStats}
        {counter > 120 && <h4>No longer automatically syncing. Please refresh page</h4>}
    </div>;
}