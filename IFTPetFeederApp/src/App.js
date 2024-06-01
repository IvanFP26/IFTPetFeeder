import React, { useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import 'react-native-gesture-handler';
import firebase from '@react-native-firebase/app';
import messaging from '@react-native-firebase/messaging';
import Router from './router';

const App = () => {
  const firebaseConfig = {
    apiKey: "AIzaSyA6R1rzEfKVWiNFu1sTIbMHkH7zQ3EYgBk",
    authDomain: "iftpetfeedercoba.firebaseapp.com",
    databaseURL: "https://iftpetfeedercoba-default-rtdb.asia-southeast1.firebasedatabase.app/",
    projectId: "iftpetfeedercoba",
    storageBucket: "iftpetfeedercoba.appspot.com",
    messagingSenderId: "827300223020",
    appId: "1:827300223020:android:15b3d07f074968687d1989",
    user_email: "petfeederift@gmail.com",
    user_password:"123456",
  };

  useEffect(() => {
    if (!firebase.apps.length) {
      firebase.initializeApp(firebaseConfig);
    }

    const getToken = async () => {
      try {
        const currentToken = await messaging().getToken();
        console.log('Current FCM Token:', currentToken);

        const databaseRef = firebase.database().ref('/fcmToken');
        const snapshot = await databaseRef.once('value');
        const storedToken = snapshot.val();

        if (storedToken !== currentToken) {
          await databaseRef.set(currentToken);
        }
      } catch (error) {
        console.error('Error getting FCM token:', error);
      }
    };

    getToken();

    const onTokenRefreshListener = messaging().onTokenRefresh(async (newToken) => {
      console.log('New FCM Token:', newToken);
      const databaseRef = firebase.database().ref('/fcmToken');
      await databaseRef.set(newToken);
    });

    return () => {
      onTokenRefreshListener();
    };
  }, []);

  useEffect(() => {
    const handleBackgroundMessage = messaging().setBackgroundMessageHandler(async remoteMessage => {
      // Tidak melakukan apa pun di sini
    });
  
    return handleBackgroundMessage;
  }, []);

  useEffect(() => {
    const databaseToken = 'TW01';
    const selectedDatabase = databaseToken === 'TW01' ? 'IFTPetFeeder1' : 'IFTPetFeeder2';
    const path = `/${selectedDatabase}/IFTPetFeeder/Schedule`;

    const onDataChange = (snapshot) => {
      const data = snapshot.val();
      if (data) {
        Object.keys(data).forEach((key) => {
          const taskData = data[key];
          sendNotification(taskData);
        });
      }
    };

    const ref = firebase.database().ref(path);
    ref.on('value', onDataChange);

    return () => {
      ref.off('value', onDataChange);
    };
  }, []);

  const sendNotification = (taskData) => {
    const { Action, day, time } = taskData;

    const currentDate = new Date();
    const currentDay = currentDate.toLocaleDateString('en-US', { weekday: 'long' });
    const currentTime = `${currentDate.getHours()}:${currentDate.getMinutes()}`;

    if (Action !== 1) {
      return;
    }

    if (day !== currentDay || time !== currentTime) {
      return;
    }

    const notificationMessage = 'Kucing sedang melakukan aktivitas (makan)';

    const notification = new firebase.notifications.Notification()
      .setNotificationId('1')
      .setTitle('IFT PETFEEDER')
      .setBody(notificationMessage)
      .setSound('default');

    notification.android.setChannelId('default_channel_id');
    notification.android.setPriority(firebase.notifications.Android.Priority.High);

    firebase.notifications().displayNotification(notification)
      .then(() => {
        console.log('Notification Sent:', notificationMessage);
      })
      .catch((error) => {
        console.error('Error sending notification:', error);
      });
  };

  return (
    <NavigationContainer>
      <Router />
    </NavigationContainer>
  );
};

export default App;