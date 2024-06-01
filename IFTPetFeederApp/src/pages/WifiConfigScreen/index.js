import React, { useState, useEffect } from 'react';
import { View, Button, TextInput, Alert, StyleSheet, PermissionsAndroid, Platform } from 'react-native';
import BleManager from 'react-native-ble-manager';

// Komponen untuk konfigurasi Wi-Fi
const WifiConfigScreen = () => {
  // State untuk menyimpan SSID, kata sandi, status koneksi perangkat, dan ID perangkat Bluetooth
  const [ssid, setSSID] = useState('');
  const [password, setPassword] = useState('');
  const [deviceConnected, setDeviceConnected] = useState(false);
  const [deviceId, setDeviceId] = useState(null);

  // Penggunaan useEffect untuk menangani permintaan izin di platform Android
  useEffect(() => {
    if (Platform.OS === 'android' && Platform.Version >= 23) {
      PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ]).then((result) => {
        if (result['android.permission.ACCESS_FINE_LOCATION'] &&
            result['android.permission.BLUETOOTH_SCAN'] &&
            result['android.permission.BLUETOOTH_CONNECT'] === PermissionsAndroid.RESULTS.GRANTED) {
          console.log('Permissions granted');
          // Hidupkan Bluetooth
          BleManager.start({ showAlert: false });
        } else {
          console.log('Permissions denied');
        }
      });
    }
  }, []);

  // Fungsi untuk memulai pemindaian perangkat Bluetooth
  const startScan = () => {
    if (!ssid.trim() || !password.trim()) {
      Alert.alert('Isi SSID dan Password', 'Harap isi SSID dan kata sandi Wi-Fi sebelum memindai perangkat.');
      return;
    }

    BleManager.scan([], 5, true)
      .then(() => {
        console.log('Scanning started...');
      })
      .catch((error) => {
        console.error('Scan failed:', error);
      });
  };

  // useEffect untuk mendengarkan peristiwa penemuan perangkat Bluetooth
  useEffect(() => {
    const handleDiscoverPeripheral = (peripheral) => {
      if (peripheral.name === 'IFTPETFEEDER') {
        BleManager.stopScan()
          .then(() => {
            console.log('Scanning stopped');
            setDeviceId(peripheral.id);
            // Terhubung ke perangkat Bluetooth yang ditemukan
            connectToDevice(peripheral.id);
          })
          .catch((error) => {
            console.error('Failed to stop scanning:', error);
          });
      }
    };

    const subscription = BleManager.addListener('BleManagerDiscoverPeripheral', handleDiscoverPeripheral);

    return () => {
      subscription.remove();
    };
  }, []);

  // Fungsi untuk terhubung ke perangkat Bluetooth yang ditemukan
  const connectToDevice = (id) => {
    BleManager.connect(id)
      .then(() => {
        console.log('Connected to', id);
        setDeviceConnected(true);
        Alert.alert('Terhubung', 'Berhasil terhubung ke perangkat.');
        // Lakukan proses pairing setelah terhubung ke perangkat Bluetooth
        pairBluetoothDevice(id);
      })
      .catch((error) => {
        console.error('Failed to connect to device:', error);
        Alert.alert('Gagal Terhubung', 'Tidak dapat terhubung ke perangkat Bluetooth.');
        setDeviceConnected(false);
      });
  };

  // Fungsi untuk memulai proses pairing Bluetooth
  const pairBluetoothDevice = (id) => {
    BleManager.pairDevice(id)
      .then((results) => {
        console.log('Hasil pairing:', results);
        if (results) {
          // Pairing berhasil, kirim konfigurasi WiFi
          sendWifiConfig();
        } else {
          // Pairing gagal
          console.log('Pairing gagal');
          Alert.alert('Gagal Pairing', 'Tidak dapat melakukan pairing dengan perangkat Bluetooth.');
        }
      })
      .catch((error) => {
        console.error('Error pairing:', error);
        Alert.alert('Gagal Pairing', 'Terdapat kesalahan saat melakukan pairing dengan perangkat Bluetooth.');
      });
  };

  // Fungsi untuk mengirim konfigurasi Wi-Fi ke perangkat Bluetooth
  const sendWifiConfig = () => {
    if (deviceId) {
      const wifiConfig = {
        ssid: ssid,
        password: password,
      };

      BleManager.writeWithoutResponse(deviceId, '10c92aef-3b4a-48f7-9cc6-c6be65a0f154', 'bfe49db5-2ee2-4028-b81a-d25100b59c07', JSON.stringify(wifiConfig))
        .then(() => {
          console.log('Wi-Fi configuration sent successfully');
          Alert.alert('Berhasil', 'Konfigurasi Wi-Fi berhasil dikirim ke perangkat.');
        })
        .catch((error) => {
          console.error('Failed to send Wi-Fi configuration:', error);
          Alert.alert('Gagal Mengirim', 'Terdapat kesalahan saat mengirim konfigurasi Wi-Fi ke perangkat.');
        });
    } else {
      Alert.alert('Perangkat Tidak Terhubung', 'Harap hubungkan ke perangkat terlebih dahulu.');
    }
  };

  // Tampilan komponen
  return (
    <View style={styles.container}>
      {/* Input untuk SSID Wi-Fi */}
      <TextInput
        style={styles.input}
        placeholder="Masukkan SSID Wi-Fi"
        onChangeText={text => setSSID(text)}
        value={ssid}
      />
      {/* Input untuk kata sandi Wi-Fi */}
      <TextInput
        style={styles.input}
        placeholder="Masukkan kata sandi Wi-Fi"
        onChangeText={text => setPassword(text)}
        value={password}
        secureTextEntry={true}
      />
      {/* Tombol untuk memulai pemindaian perangkat */}
      <View style={styles.buttonContainer}>
        <Button title="Pindai Perangkat" onPress={startScan} color="#F0743E" />
      </View>
    </View>
  );
};

// Stylesheet untuk komponen
const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 20,
    backgroundColor: 'white', // Latar belakang putih
  },
  input: {
    marginBottom: 10,
    width: '83%',
    height: 40,
    borderColor: 'gray',
    borderWidth: 2,
    paddingLeft: 8,
    borderRadius: 5,
  },
  buttonContainer: {
    marginTop: 20,
    borderRadius: 50,
    paddingVertical: 10,
  },
});

export default WifiConfigScreen;
