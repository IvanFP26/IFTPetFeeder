import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Image, Alert } from 'react-native';
import DateTimePicker from '@react-native-community/datetimepicker';
import { Dropdown } from 'react-native-element-dropdown';
import { useNavigation } from '@react-navigation/native';
import firebase from '@react-native-firebase/app';
import '@react-native-firebase/database';
import AsyncStorage from '@react-native-async-storage/async-storage';

const foodOptions = [
  { label: 'Staple Food', value: 'Staple' },
  { label: 'Snack Food', value: 'Snack' },
];

const portionOptions = Array.from({ length: 20 }, (_, i) => ({ label: `${(i + 1) * 5}`, value: (i + 1) * 5 }));

const dayOptions = [
  { label: 'Everyday', value: 'Everyday' },
  { label: 'Monday', value: 'Monday' },
  { label: 'Tuesday', value: 'Tuesday' },
  { label: 'Wednesday', value: 'Wednesday' },
  { label: 'Thursday', value: 'Thursday' },
  { label: 'Friday', value: 'Friday' },
  { label: 'Saturday', value: 'Saturday' },
  { label: 'Sunday', value: 'Sunday' },
];

const SchedulingScreen = () => {
  const [date, setDate] = useState(new Date());
  const [selectedFood, setSelectedFood] = useState(null);
  const [selectedPortion, setSelectedPortion] = useState(null);
  const [selectedDay, setSelectedDay] = useState(null);
  const [showTimePicker, setShowTimePicker] = useState(false);
  const [selectedDatabase, setSelectedDatabase] = useState('');
  const [isScheduling, setIsScheduling] = useState(false);
  const [isValidationError, setIsValidationError] = useState(false);
  const navigation = useNavigation();

  useEffect(() => {
    const fetchDatabaseToken = async () => {
      try {
        const token = await AsyncStorage.getItem('userToken');
        let database = '';
        if (token === 'TW01') {
          database = 'IFTPetFeeder1';
        } else if (token === 'PO20') {
          database = 'IFTPetFeeder2';
        }
        setSelectedDatabase(database);
      } catch (error) {
        console.error('Error fetching database token:', error);
      }
    };

    fetchDatabaseToken();
  }, []);

  const handleDoneButtonPress = async () => {
    if (!selectedFood || !selectedPortion || !selectedDay) {
      setIsValidationError(true);
      return;
    }

    if (isScheduling) {
      return; // Jika proses penjadwalan sedang berlangsung, keluar dari fungsi
    }

    setIsScheduling(true); // Mulai proses penjadwalan

    try {
      const scheduleRef = firebase.database().ref(`/${selectedDatabase}/IFTPetFeeder/Schedule`);

      const snapshot = await scheduleRef.once('value');
      const schedules = snapshot.val();
      const taskCount = schedules ? Object.keys(schedules).length : 0;

      // Mengecek apakah sudah mencapai batas task
      if (taskCount >= 10) {
        Alert.alert("Hapus salah satu jadwal untuk menambahkan yang baru");
        return;
      }

      // Mendapatkan nomor task terakhir
      let lastTaskNumber = taskCount > 0 ? taskCount : 1;

      // Jika ada task yang dihapus, perbarui urutan nomor task yang tersisa
      if (taskCount > 0) {
        // Mendapatkan urutan nomor task yang tersedia
        const availableTaskNumbers = Object.keys(schedules).map(task => parseInt(task.replace('Task', '')));

        // Mencari nomor task terkecil yang belum digunakan
        let smallestAvailableTaskNumber = 1;
        while (availableTaskNumbers.includes(smallestAvailableTaskNumber)) {
          smallestAvailableTaskNumber++;
        }

        // Menentukan nomor task terakhir berdasarkan nomor terkecil yang tersedia
        lastTaskNumber = smallestAvailableTaskNumber;
      }

      const hours = date.getHours().toString().padStart(2, '0');
      const minutes = date.getMinutes().toString().padStart(2, '0');

      // Menyimpan data jadwal baru ke Firebase dengan nomor task terakhir
      await scheduleRef.child(`Task${lastTaskNumber}`).set({
        Time: `${hours}:${minutes}`, // Jam dan menit
        Action: 1, // Langsung aktifkan jadwal baru
        Food: selectedFood || "", // Jika selectedFood null, set ke string kosong
        Gram: selectedPortion || "", // Jika selectedPortion null, set ke string kosong
        Day: selectedDay || "", // Jika selectedDay null, set ke string kosong
      });

      resetSelections();

      navigation.navigate('Scheduling');

      console.log("Schedule set successfully!");
    } catch (error) {
      console.error("Error setting schedule:", error);
    } finally {
      setIsScheduling(false); // Selesai proses penjadwalan
    }
  };

  const resetSelections = () => {
    setSelectedFood(null);
    setSelectedPortion(null);
    setSelectedDay(null);
  };

  return (
    <View style={{ backgroundColor: "white", flex: 1 }}>
      <TouchableOpacity style={styles.backButton} onPress={() => navigation.navigate('Scheduling')}>
        <Image source={require('../../assets/icon/arrow_back.png')} style={styles.backIcon} />
      </TouchableOpacity>
      <View style={styles.container}>
        <Text style={styles.title}>Scheduling</Text>
      </View>

      <View style={styles.datePickerContainer}>
        <TouchableOpacity onPress={() => setShowTimePicker(true)}>
          <Text style={styles.dateText}>{`${date.getHours()}:${date.getMinutes().toString().padStart(2, '0')}`}</Text>
        </TouchableOpacity>
        {showTimePicker && (
          <DateTimePicker
            value={date}
            mode="time"
            is24Hour={true}
            display="spinner"
            onChange={(event, selectedDate) => {
              const currentDate = selectedDate || date;
              setShowTimePicker(false);
              setDate(currentDate);
            }}
          />
        )}
      </View>

      <View style={styles.container1}>
        <Dropdown
          style={styles.dropdown}
          data={foodOptions}
          maxHeight={300}
          labelField="label"
          valueField="value"
          placeholder="Select Food"
          value={selectedFood}
          onChange={(item) => setSelectedFood(item.value)}
        />

        <Dropdown
          style={styles.dropdown}
          data={portionOptions}
          maxHeight={300}
          labelField="label"
          valueField="value"
          placeholder="Portions"
          value={selectedPortion}
          onChange={(item) => setSelectedPortion(item.value)}
        />

        <Dropdown
          style={styles.dropdown}
          data={dayOptions}
          maxHeight={300}
          labelField="label"
          valueField="value"
          placeholder="Day"
          value={selectedDay}
          onChange={(item) => setSelectedDay(item.value)}
        />
      </View>

      {isValidationError && (
        <Text style={styles.errorText}>Please select food, portion, and day!</Text>
      )}

      <View style={{ flexDirection: 'row', justifyContent: 'center' }}>
        <TouchableOpacity 
          style={[styles.button, isScheduling && styles.disabledButton]} 
          onPress={handleDoneButtonPress} 
          disabled={isScheduling}
        >
          <Text style={styles.text4}>DONE</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    height: 32,
    width: 132,
    marginRight: 103,
    marginLeft: 120,
    marginTop: 24,
    textAlign: 'center',
    flexDirection: 'row',
    justifyContent: 'center'
  },
  container1: {
    flex: 1,
    backgroundColor: 'white',
    padding: 16,
    marginTop: 20,
  },
  title: {
    color: '#0B1C3F',
    fontSize: 22,
    textAlign: 'center',
    fontFamily: 'Roboto',
    marginBottom: 2,
    marginRight: 2,
    marginLeft:1,
  },
  button: {
    width: 240,
    height: 45,
    borderRadius: 25,
    marginTop: 30,
    marginBottom: 20,
    backgroundColor: '#8F7D6F',
    alignItems: 'center',
    justifyContent: 'center',
  },
  text4: {
    color: '#FFFFFF',
    fontSize: 18,
  },
  dropdown: {
    height: 50,
    borderColor: '#0B1C3F',
    borderWidth: 1,
    borderRadius: 8,
    paddingHorizontal: 8,
    marginBottom: 20,
  },
  datePickerContainer: {
    alignItems: 'center',
    marginTop: 20,
  },
  dateText: {
    fontSize: 18,
    color: '#0B1C3F',
    textAlign: 'center',
    marginLeft: 22,
  },
  backButton: {
    position: 'absolute',
    top: 20,
    left: 10,
  },
  backIcon: {
    width: 30,
    height: 20,
    marginTop: 10,
    marginLeft: 5,
  },
  errorText: {
    color: 'red',
    fontSize: 16,
    textAlign: 'center',
    marginBottom: 10,
  },
  disabledButton: {
    backgroundColor: '#D3D3D3', // Warna tombol saat dinonaktifkan
  },
});

export default SchedulingScreen;
