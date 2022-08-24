import React, {useState } from 'react';
//import axios from 'axios';
import * as Realm from 'realm-web';

export default ({ statsId }) => {
    const [new_name, set_New_name] = useState('');

    const onSubmit = async (event) => {
        event.preventDefault();

        //Axios version
        /*
        await axios.post(process.env.REACT_APP_BACKEND_URL + `stats/${statsId}`, {
            "name": new_name
        });
        */

        //use Realm to get data from Realm
        //environmental variables aren't working for some reason: process.env.REALM_APP_ID
        const app = new Realm.App({id: 'laser-tag-1-wmptj'});
        const credentials = Realm.Credentials.anonymous();
        const user = await app.logIn(credentials);
        const current_stats = await user.functions.UpdateName({"id": statsId.toString(), "name": new_name});

        set_New_name('');
    };
    
    return <div>
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
    </div>;
};