// Router.js

import React from "react";
import { createStackNavigator } from '@react-navigation/stack';
import { Homescreen, Splashscreen, Historyscreen, Feedscreen, Schedulingscreen, Timescreen, Loginscreen} from "../pages";
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { BottomNavigator } from '../components/molecules';

const Stack = createStackNavigator();
const Tab = createBottomTabNavigator();

const MainApp = () => {
    return (
        <Tab.Navigator tabBar={props => <BottomNavigator {...props} />} initialRouteName="Home">
            <Tab.Screen name="Home" component={Homescreen} options={{ headerShown: false }} />
            <Tab.Screen name="Scheduling" component={Timescreen} options={{ headerShown: false }} />
            <Tab.Screen name="Feed" component={Feedscreen} options={{ headerShown: false }} />
            <Tab.Screen name="History" component={Historyscreen} options={{ headerShown: false }} />
        </Tab.Navigator>
    )
}

const Router = () => {
    return (
        <Stack.Navigator>
            <Stack.Screen name="Splashscreen" component={Splashscreen} options={{ headerShown: false }} />
            <Stack.Screen name="MainApp" component={MainApp} options={{ headerShown: false }} />
            <Stack.Screen name="Timescreen" component={Schedulingscreen} options={{ headerShown: false }} />
            <Stack.Screen name="Loginscreen" component={Loginscreen} options={{ headerShown: false }} />
        </Stack.Navigator>
    );
};

export default Router;
