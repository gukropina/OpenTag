import React from 'react';
import {Link} from 'react-router-dom';
import MainHeader from './MainHeader';
import NavLinks from './NavLinks';
/*
import SideDrawer from './SideDrawer';
import Backdrop from './Backdrop';
import LeaveButton from './LeaveButton';
*/

import './MainNavigation.css';

const MainNavigation = props => {
 /*
    const [drawerIsOpen, setDrawerIsOpen] = useState(false);

    const openDrawer = () => {
        setDrawerIsOpen(true);
        console.log('clicked open');
    };

    const closeDrawer = () => {
        setDrawerIsOpen(false);
        console.log('clicked close');
    };
    */

    return (
        <React.Fragment>
            <MainHeader>
            <button className="main-nagigation__menu-btn"> 
                <span />
                <span />
                <span />
            </button>
            <h1 className="main-navigatioin__title"> 
                <Link to="/">Laser Tag</Link>
            </h1>
            <nav className="main-navigation__header-nav">
                <NavLinks />
            </nav>
        </MainHeader>
    </React.Fragment>
    );
};

export default MainNavigation; 