import React, {useState, useEffect } from 'react';
import axios from 'axios';

export default () => {
    const [events, setEvents] = useState([]);

    //get list of all events (note, your node.js app limits the number of events to 50 for now)
    const fetchEvents = async () => {
        const res = await axios.get('http://localhost:4001/events');

        setEvents(res.data);
    };

    //Apply some style!

    useEffect(() => {
        fetchEvents();
    }, []);

    const renderedEvents = events.map(events => {
        return <div 
            className="card" 
            style={{ width: '100%', marginBottom: '20px' }}
            key={events._id}
            >
                <div className="card-body">
                    <h3>{events.event_name}</h3>
                    <ul>
                        <li>Device ID tagged: {events.who_was_hit}</li>
                        <li>Tagged by Device: {events.hit_by}</li>
                        <li>Tag Received: {events.tag_name}</li>
                        <li>Tagger Team: {events.team_name}</li>
                        <li>Tag Bonus Damage: {events.bonus_damage}</li>
                        <li>Tag Checksum Matched: {events.checksum_matches}</li>
                        <li>Event Time: {events.createdAt}</li>
                    </ul>
                </div>
        </div>
    });
    

    return <div className="d-flex flex-row flex-wrap justify-content-between">
        {renderedEvents}
    </div>;
}