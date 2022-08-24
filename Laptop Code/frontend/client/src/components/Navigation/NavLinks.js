import React from 'react';
import { NavLink } from 'react-router-dom';

import './NavLinks.css';

const NavLinks = props => {
    return <ul className="nav-links">
        <li>
            <NavLink to="/" exact>Stats</NavLink>
        </li>
        <li>
            <NavLink to="/events">Events</NavLink>
        </li>
        <li>
            <NavLink to="/commands">Commands</NavLink>
        </li>
        <li>
            <NavLink to="/login" >Login</NavLink>
        </li>
    </ul>
};

export default NavLinks;