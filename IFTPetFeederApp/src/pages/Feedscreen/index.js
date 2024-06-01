import React, { useState, useEffect } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, Alert } from 'react-native';
import DropDownPicker from 'react-native-dropdown-picker';
import RNPickerSelect from 'react-native-picker-select';
import firebase from '@react-native-firebase/app';
import '@react-native-firebase/database';
import AsyncStorage from '@react-native-async-storage/async-storage';

const Feedscreen = () => {
  const [currentTime, setCurrentTime] = useState(new Date());
  const [selectedFood, setSelectedFood] = useState(null);
  const [open, setOpen] = useState(false);
  const [portion, setPortion] = useState(null);
  const [items, setItems] = useState([
    { label: 'Staple Food', value: 'Staple' },
    { label: 'Snack Food', value: 'Snack' },
  ]);

  const [databaseToken, setDatabaseToken] = useState('');
  const [selectedDatabase, setSelectedDatabase] = useState('');
  const [isFeeding, setIsFeeding] = useState(false); // State untuk menandai apakah proses pemberian makanan sedang berlangsung

  useEffect(() => {
    const fetchDatabaseToken = async () => {
      try {
        const token = await AsyncStorage.getItem('userToken');
        setDatabaseToken(token);
        if (token === 'TW01') {
          setSelectedDatabase('IFTPetFeeder1');
        } else if (token === 'PO20') {
          setSelectedDatabase('IFTPetFeeder2');
        }
      } catch (error) {
        console.error('Error fetching database token:', error);
      }
    };
  
    fetchDatabaseToken();
  }, []);

  useEffect(() => {
    const interval = setInterval(() => {
      setCurrentTime(new Date());
    }, 1000);

    return () => clearInterval(interval);
  }, []);

  const handleManualFeeder = () => {
    if (isFeeding) {
      Alert.alert("Proses pemberian makanan sedang berlangsung, mohon tunggu...");
      return;
    }

    if (!selectedFood) {
      Alert.alert("Pilih jenis makanan terlebih dahulu!");
      return;
    }

    if (!portion) {
      Alert.alert("Pilih jumlah porsi terlebih dahulu!");
      return;
    }

    if (!databaseToken) {
      Alert.alert("Token database tidak tersedia!");
      return;
    }

    let storageRef;
    if (selectedFood === 'Staple') {
      storageRef = `/${selectedDatabase}/IFTPetFeeder/Storage/Staple`; 
    } else if (selectedFood === 'Snack') {
      storageRef = `/${selectedDatabase}/IFTPetFeeder/Storage/Snack`; 
    }

    setIsFeeding(true); // Tandai bahwa proses pemberian makanan sedang berlangsung

    firebase.database().ref(storageRef).once('value', snapshot => {
      const foodAmount = snapshot.val();
      if (foodAmount <= 0) {
        Alert.alert("Isi ulang makanan!!!");
      } else {
        firebase.database().ref(`${selectedDatabase}/IFTPetFeeder/Feeding/Check`).set(1);
        const portionInt = parseInt(portion);
        firebase.database().ref(`${selectedDatabase}/IFTPetFeeder/Feeding/Gram`).set(portionInt);
        firebase.database().ref(`${selectedDatabase}/IFTPetFeeder/Feeding/FoodPosition`).set(selectedFood);

        setSelectedFood(null);
        setPortion(null);
      }

      setIsFeeding(false); // Set kembali status menjadi false setelah proses selesai
    });
  };

 return (
    <View style={styles.container}>
      {/* Manual Feeder Title */}
      <Text style={styles.title}>Manual Feeder</Text>
      {/* Circle Container */}
      <View style={styles.circleContainer}>
        {/* SET HERE Text */}
        <Text style={styles.text4}>SET HERE</Text>
        {/* Clock */}
        <Text style={styles.clockText}>{currentTime.toLocaleTimeString()}</Text>
        {/* DropDown Picker */}
        <View style={styles.pickerContainer}>
          <DropDownPicker
            style={styles.dropDownPicker}
            open={open}
            value={selectedFood}
            items={items}
            setOpen={setOpen}
            setValue={setSelectedFood}
            setItems={setItems}
            boxStyles={{ marginHorizontal: 10, borderRadius: 15, textColor: "#0B1C3F", backgroundColor: "#FFFFFF", width: 120, height: 50 }}
          />
        </View>
      </View>
      {/* Feed Portion(s) */}
      <View style={styles.feedPortionContainer}>
        <Text style={styles.text1}> Feed Portion(s) </Text>
        <RNPickerSelect
          onValueChange={(value) => setPortion(value)}
          items={[
            { label: '5 gram', value: '5' },
            { label: '10 gram', value: '10' },
            { label: '15 gram', value: '15' },
            { label: '20 gram', value: '20' },
            { label: '25 gram', value: '25' },
            { label: '30 gram', value: '30' },
            { label: '35 gram', value: '35' },
            { label: '40 gram', value: '40' },
            { label: '45 gram', value: '45' },
            { label: '50 gram', value: '50' },
            { label: '55 gram', value: '55' },
            { label: '60 gram', value: '60' },
            { label: '65 gram', value: '65' },
            { label: '70 gram', value: '70' },
            { label: '75 gram', value: '75' },
            { label: '80 gram', value: '80' },
            { label: '85 gram', value: '85' },
            { label: '90 gram', value: '90' },
            { label: '95 gram', value: '95' },
            { label: '100 gram', value: '100' },
          ]}
          style={pickerStyles}
          placeholder={{ label: 'Pilih gram', value: null, color: 'black' }} 
          value={portion}
        />
      </View>
      {/* Manual Feeder Button */}
      <View style={styles.manualFeederButtonContainer}>
        <TouchableOpacity style={styles.manualFeederButton} onPress={handleManualFeeder}>
          <Text style={styles.text3}> Manual Feeder</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
};

const pickerStyles = {
  inputIOS: {
    fontSize: 18,
    paddingVertical: 12,
    paddingHorizontal: 10,
    borderWidth: 1,
    borderColor: 'gray',
    borderRadius: 4,
    color: 'black',
    paddingRight: 30, // to ensure the text is never behind the icon
    marginLeft: 140,
    marginTop: 10,
    placeholderTextColor: 'gray'
  },
  inputAndroid: {
    fontSize: 18,
    paddingHorizontal: 10,
    paddingVertical: 8,
    borderWidth: 0.5,
    borderColor: 'purple',
    borderRadius: 8,
    color: 'black',
    paddingRight: 30, // to ensure the text is never behind the icon
    placeholderTextColor: 'gray'
  },
};

const styles = StyleSheet.create({
  container: {
    backgroundColor: "#FFFFFF",
    flex: 1,
    justifyContent: "center",
    alignItems: "center",
    textAlign: 'center',
  },
  title: {
    color: '#0B1C3F',
    fontSize: 22,
    marginBottom: 60,
    fontFamily: 'Roboto',
    textAlign: 'center',
  },
  circleContainer: {
    alignItems: "center",
    justifyContent: "center",
    width: 280,
    height: 280,
    borderRadius: 280/2,
    backgroundColor: '#8F7D6F',
    textAlign: 'center',
  },
  text4: {
    fontSize: 18,
    marginBottom: 25,
    color :"#FFFFFF",
    fontWeight : 'bold'
  },
  clockText: {
    color: '#FFFFFF',
    fontSize: 20,
    marginBottom: 10,
    fontWeight : 'bold'
  },
  pickerContainer: {
    alignItems: 'center',
    marginTop: 10,
    width: 200,
  },
  dropDownPicker: {
    width: '100%',
  },
  feedPortionContainer: {
    alignItems: "center",
    marginTop: 20,
  },
  text1: {
    fontSize: 18,
    marginBottom: 10,
    color: '#0B1C3F',
  },
  manualFeederButtonContainer: {
    alignItems: "center",
    marginTop: 50,
  },
  manualFeederButton: {
    width: 250,
    height: 50,
    borderRadius: 25,
    backgroundColor: '#8F7D6F',
    justifyContent: "center",
    alignItems: "center",
  },
  text3: {
    color: "#FFFFFF",
    fontSize: 18,
    fontWeight : 'bold'
  },
});

export default Feedscreen;
