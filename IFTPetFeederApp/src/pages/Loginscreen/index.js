import React, { useState, useEffect } from 'react';
import { View, TextInput, TouchableOpacity, Image, Text, Alert, Linking } from 'react-native';
import firebase from '@react-native-firebase/app';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { ic_launcher, eye_open, eye_close } from '../../assets/image';
import { useNavigation } from '@react-navigation/native';

const Loginscreen = () => {
    const [token, setToken] = useState('');
    const [showToken, setShowToken] = useState(false);
    const navigation = useNavigation();

    useEffect(() => {
        checkLoginStatus();
    }, []);

    const checkLoginStatus = async () => {
        try {
            const userToken = await AsyncStorage.getItem('userToken');
            if (userToken) {
                navigation.navigate('MainApp');
            }
        } catch (error) {
            console.error('Error retrieving user token:', error);
        }
    };

    const handleLogin = async () => {
        if (token.trim() === '') {
            Alert.alert('Error', 'Please enter your token');
            return;
        }

        const databaseRef1 = firebase.database().ref('/IFTPetFeeder1/Token');
        const databaseRef2 = firebase.database().ref('/IFTPetFeeder2/Token');

        let databaseRef;
        if (token === "TW01") {
            databaseRef = databaseRef1;
        } else if (token === "PO20") {
            databaseRef = databaseRef2;
        } else {
            Alert.alert('Error', 'Invalid token');
            return;
        }

        try {
            const snapshot = await databaseRef.once('value');
            const databaseToken = snapshot.val();
            if (token === databaseToken) {
                await AsyncStorage.setItem('userToken', token);
                navigation.navigate('MainApp');
            } else {
                Alert.alert('Error', 'Invalid token');
            }
        } catch (error) {
            console.error('Login error:', error);
            Alert.alert('Error', 'An error occurred, please try again later');
        }
    };

    const openWhatsApp = () => {
        Linking.openURL('https://api.whatsapp.com/send?phone=6281365010144');
    };

    return (
        <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center', backgroundColor: '#FFFFFF' }}>
            <Image source={ic_launcher} style={{ width: 189, height: 187, marginBottom: 40 }} />
            <View style={{ flexDirection: 'row', alignItems: 'center', width: '80%', borderColor: 'gray', borderWidth: 1, borderRadius: 5, marginBottom: 30 }}>
                <TextInput
                    style={{ height: 50, width: '85%', paddingHorizontal: 10 }}
                    placeholder="Enter token"
                    onChangeText={text => setToken(text)}
                    value={token}
                    secureTextEntry={!showToken}
                />
                <TouchableOpacity onPress={() => setShowToken(!showToken)} style={{ padding: 10 }}>
                    <Image source={showToken ? eye_open : eye_close} style={{ width: 20, height: 20 }} />
                </TouchableOpacity>
            </View>
            <TouchableOpacity style={{ backgroundColor: '#F0743E', paddingHorizontal: 125, paddingVertical: 14, borderRadius: 10 }} onPress={handleLogin}>
                <Text style={{ color: 'white', fontSize :18 }}>Login</Text>
            </TouchableOpacity>
            <TouchableOpacity onPress={openWhatsApp}>
                <Text style={{ marginTop: 40, color: '#007bff' }}>Forgot token? Contact call center</Text>
            </TouchableOpacity>
        </View>
    );
};

export default Loginscreen;
