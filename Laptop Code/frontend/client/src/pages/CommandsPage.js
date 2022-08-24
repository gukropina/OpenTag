import React from 'react';
import CommandsList from '../components/CommandsList';

const CommandsPage = () => {
    return (
        <div className="container">
        <CommandsList />
        <hr />
        <h2>Commands</h2>
            <ul>
              <li key={1}>1: Survival Mode On</li>
              <li key={2}>2: Survival Mode Off</li>
              <li key={3}>3: Auto-revive On</li>
              <li key={4}>4: Auto-revive Off</li>
              <li key={5}>5: Ammo Use On</li>
              <li key={6}>6: Ammo Use Off</li>
              <li key={7}>7: Aim Assist On</li>
              <li key={8}>8: Aim Assist Off</li>
              <li key={9}>9: Give Double Damage Rune</li>
              <li key={10}>10: Give Double Health Rune</li>
              <li key={11}>11: Give Arcane Rune</li>
              <li key={12}>12: Give Double Armor Rune</li>
              <li key={13}>13: Give Extra Life Rune</li>
              <li key={14}>14: Give 50% Ammo refill</li>
            </ul>
            <h6>Stats automatically refresh for 10 minutes after loading the page. 
                If stats aren't refreshing automatically, please refresh the page</h6>
            </div>
        );
}

export default CommandsPage;