/*
So, you made the app on your C drive instead of on the cloud drive. Oh well.
To start the app, open a terminal
change directory to the C:/users/gukro/documents/open-tag-laptop-code\frontend\client and run npm start to
  start the server (can use terminal here)
This will automatically open up a webpage to view the app

control C to stop the server

use npm run build to make a build of your site to send to Netlify
*/
import React from 'react';
import { BrowserRouter as Router, Route, Redirect, Switch } from 'react-router-dom';

import ViewStats from './pages/ViewStats';
import EventsPage from './pages/EventsPage';
import CommandsPage from './pages/CommandsPage';
import MainNavigation from './components/Navigation/MainNavigation';

export default () => {
    
  return (
      <Router>
        <MainNavigation />
        <main>
          <Switch>
            <Route path="/" exact >
              <ViewStats />
            </Route>
            <Route path="/events" exact >
              <EventsPage />
            </Route>
            <Route path="/commands" exact >
              <CommandsPage />
            </Route>
            <Redirect to="/" />
          </Switch>
        </main>
      </Router>
    );
};