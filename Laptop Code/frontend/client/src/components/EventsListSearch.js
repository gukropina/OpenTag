import React, {useState, useEffect } from 'react';
import axios from 'axios';
import * as Realm from 'realm-web';

export default () => {
    /*
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
    */
    const [events, setEvents] = useState([]);

    const fetchEvents = async () => {
        
        const res = await axios.get(process.env.REACT_APP_BACKEND_URL + 'events');
        setEvents(res.data);

        //use Realm to get data from Realm
        //environmental variables aren't working for some reason: process.env.REALM_APP_ID
        /*
        const app = new Realm.App({id: 'laser-tag-1-wmptj'});
        const credentials = Realm.Credentials.anonymous();
        const user = await app.logIn(credentials);
        const events_result = await user.functions.GetAllEvents();
        console.log(events_result);
        setEvents(events_result);
        */

        
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

    //variables for searching who was hit
    const [new_who_was_hit_search, set_New_who_was_hit_search] = useState('');


    const onSubmit = async (event) => {
        event.preventDefault();
        if(!isNaN(+new_who_was_hit_search)){
            //if the entered information is a number
            const res = await axios.get(process.env.REACT_APP_BACKEND_URL + `events/who_was_hit/${new_who_was_hit_search}`, {});

            set_New_who_was_hit_search('');
            setEvents(res.data);
        }
        else{
            set_New_who_was_hit_search('invalid entry');
        }
        
    };

    //variables for searching who hit you
    const [new_hit__by_search, set_New_hit_by_search] = useState('');


    const onSubmit_hitBy = async (event) => {
        event.preventDefault();
        if(!isNaN(+new_who_was_hit_search)){
            //if the entered information is a number
            const res = await axios.get(process.env.REACT_APP_BACKEND_URL + `events/hit_by/${new_hit__by_search}`, {});

            set_New_hit_by_search('');
            setEvents(res.data);
        }
        else{
            set_New_hit_by_search('invalid entry');
        }
    };
    

    return (
        <React.Fragment>
            <form onSubmit={onSubmit}>
                <div className="form-group">
                    <label>Search by Device ID that was Hit</label>
                    <input 
                        value={new_who_was_hit_search} 
                        onChange={e => set_New_who_was_hit_search(e.target.value)} 
                        className="form-control" 
                    />
                </div>
                <button className="btn btn-primary">Submit</button>
            </form>
            <form onSubmit={onSubmit_hitBy}>
                <div className="form-group">
                    <label>Search by Device ID that Hit Someone else</label>
                    <input 
                        value={new_hit__by_search} 
                        onChange={e => set_New_hit_by_search(e.target.value)} 
                        className="form-control" 
                    />
                </div>
                <button className="btn btn-primary">Submit</button>
            </form>
            <div className="d-flex flex-row flex-wrap justify-content-between">
                {renderedEvents}
            </div>
        </React.Fragment>
    );
}