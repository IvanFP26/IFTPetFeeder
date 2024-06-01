import React, { useEffect, useState } from 'react';
import { Image, StyleSheet, View } from 'react-native';
import { tampilanutama1 } from '../../assets/image';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { useNavigation } from '@react-navigation/native';

const Splashscreen = () => {
  const navigation = useNavigation();
  const [isSplashTimeout, setIsSplashTimeout] = useState(false);

  // Set timeout untuk layar splash
  useEffect(() => {
    const timer = setTimeout(() => {
      setIsSplashTimeout(true); // Set timeout layar splash
    }, 1970);

    return () => clearTimeout(timer); // Hapus timer saat komponen unmount
  }, []);

  // Periksa status login setelah layar splash selesai
  useEffect(() => {
    const checkLoginStatus = async () => {
      try {
        const userToken = await AsyncStorage.getItem('userToken');
        if (userToken) {
          navigation.replace('MainApp'); // Jika sudah login, navigasi ke MainApp
        } else {
          // Jika belum login, tunggu input token
          const tokenTimer = setTimeout(() => {
            navigation.replace('Loginscreen');
          }, 190);
          // Hapus timer saat komponen unmount atau saat sudah login
          return () => clearTimeout(tokenTimer);
        }
      } catch (error) {
        console.error('Error checking login status:', error);
        navigation.replace('Loginscreen');
      }
    };

    if (isSplashTimeout) {
      checkLoginStatus(); // Setelah 3 detik, periksa status login
    }
  }, [isSplashTimeout, navigation]);

  return (
    <View style={styles.container}>
      <Image source={tampilanutama1} />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#FFFFFF',
  },
});

export default Splashscreen;
