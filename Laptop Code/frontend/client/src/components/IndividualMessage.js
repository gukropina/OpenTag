import React, {useState } from 'react';
import axios from 'axios';

export default ({ deviceID }) => {
    const [new_command, set_New_command] = useState('');

    const onSubmit = async (event) => {
        event.preventDefault();
        if(!isNaN(+new_command)){
            //if the entered information is a number
            await axios.post(process.env.REACT_APP_BACKEND_URL + `command/${deviceID}`, {
                "command": new_command
            });

            set_New_command('');
        }
        else{
            set_New_command('invalid entry');
        }
    };
    
    return <div>
        <form onSubmit={onSubmit}>
            <div className="form-group">
                <label>Send Command</label>
                <input 
                    value={new_command} 
                    onChange={e => set_New_command(e.target.value)} 
                    className="form-control" 
                />
            </div>
            <button className="btn btn-primary">Send Command</button>
        </form>
    </div>;
};