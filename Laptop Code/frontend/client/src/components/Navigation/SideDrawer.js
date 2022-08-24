import React from 'react'; 
import ReactDOM from 'react-dom';

import './SideDrawer.css';

 
const SideDrawer = props => {
    const content = <aside className="side-drawer" >
        <button className="btn btn-primary" onClick={props.OnClick}>Remove</button>
        {props.children}
    </aside>;

    return ReactDOM.createPortal(content, document.getElementById('drawer-hook'));
};

export default SideDrawer;