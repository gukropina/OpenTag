import React, {useState, useEffect } from 'react';
import axios from 'axios';

export default ({ statsId }) => {
    /*
     const [events, setEvents] = useState([]);

    const fetchEvents = async () => {
        const res = await axios.get('http://localhost:4001/events');

        setEvents(res.data);
    };

    //Apply some style!

    useEffect(() => {
        fetchEvents();
    }, []);
    */
    const [stats, setStats] = useState([]);

    //get list of all stats for a particular id (note, your node.js app limits the number of events to 50 for now)
    const fetchStats = async () => {
        const res = await axios.get('http://localhost:4001/stats/${statsId}');

        setStats(res.data);
    };

    //Apply some style!

    useEffect(() => {
        fetchStats();
    }, []);
    
    const [new_name, set_New_name] = useState('');

    const onSubmit = async (event) => {
        event.preventDefault();
        await axios.post(`http://localhost:4001/stats/${statsId}`, {
            "name": new_name
        });

        set_New_name('');
    };
    
    return (
        <div 
        className="card" 
        style={{ width: '30%', marginBottom: '20px' }}
        key={stats._id}>
            <h3>{stats.name}</h3>
            <ul>
                <li style={{backgroundColor: stats.alive_or_dead === "alive" ? "lightgreen" : "lightpink"}}>Status: {stats.alive_or_dead}</li>
                <li>Kills: {stats.kills}</li>
                <li>Deaths: {stats.deaths}</li>
                <li>Shots: {stats.shots}</li>
                <li>Hits: {stats.hits}</li> 
                <li>Device Id: {stats.device_id}</li>
                <li>Game: {stats.game}</li>
            </ul>
            <div>
                <form onSubmit={onSubmit}>
                    <div className="form-group">
                        <label>Change Name</label>
                        <input 
                            value={new_name} 
                            onChange={e => set_New_name(e.target.value)} 
                            className="form-control" 
                        />
                    </div>
                    <button className="btn btn-primary">Submit</button>
                </form>
            </div>
        </div>
    );
};