import React from 'react';
//import EventsList from '../components/EventsList';
import EventsListSearch from '../components/EventsListSearch';

const EventsPage = () => {
    return (
        <div className="container">
            <h2>Game Events</h2>
            <EventsListSearch />
        </div>
        );
}

export default EventsPage;