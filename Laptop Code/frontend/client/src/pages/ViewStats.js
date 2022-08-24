import React from 'react';
import StatsList from '../components/StatsList';

const ViewStats = () => {
    return (
        <div className="container">
            <h2>Player Stats</h2>
            <StatsList />
            <h6>Stats automatically refresh for 10 minutes after loading the page. 
                If stats aren't refreshing automatically, please refresh the page</h6>
        </div>
    )
}

export default ViewStats;