import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, Switch, Image, TouchableOpacity, Modal, ScrollView, RefreshControl, Animated, Easing } from 'react-native'; // Tambahkan ScrollView dan RefreshControl
import firebase from '@react-native-firebase/app';
import '@react-native-firebase/database';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { Fotoprofil, tutup_atas, tutup_bawah, line_kiri, line_kanan } from '../../assets/image';
import { useNavigation } from '@react-navigation/native';

const Header = () => {
  const navigation = useNavigation();
  const [isEnabled, setIsEnabled] = useState(false);
  const [kucingImageURL, setKucingImageURL] = useState('');
  const [snackAmountToday, setSnackAmountToday] = useState(0);
  const [stapleAmountToday, setStapleAmountToday] = useState(0);
  const [gramAmountToday, setGramAmountToday] = useState(0);
  const [lastAte, setLastAte] = useState('');
  const [snackAmountStorage, setSnackAmountStorage] = useState(0);
  const [stapleAmountStorage, setStapleAmountStorage] = useState(0);
  const [databaseToken, setDatabaseToken] = useState('');
  const [refreshing, setRefreshing] = useState(false); // Tambahkan state refreshing
  const [showLogoutModal, setShowLogoutModal] = useState(false);
  const fadeAnim = useState(new Animated.Value(0))[0];
  const scaleAnim = useState(new Animated.Value(0))[0];
 
  useEffect(() => {
    const fetchDatabaseToken = async () => {
      try {
        const token = await AsyncStorage.getItem('userToken');
        setDatabaseToken(token);
      } catch (error) {
        console.error('Error fetching database token:', error);
      }
    };
    
    fetchDatabaseToken();
  }, []);

  const handleLogout = () => {
    Animated.timing(fadeAnim, {
      toValue: 1,
      duration: 50,
      useNativeDriver: true,
      easing: Easing.ease,
    }).start();
    
    Animated.timing(scaleAnim, {
      toValue: 1,
      duration: 50,
      useNativeDriver: true,
      easing: Easing.ease,
    }).start(() => {
      setShowLogoutModal(true);
    });
  };

  const confirmLogout = async () => {
    try {
      await AsyncStorage.removeItem('userToken');
      navigation.replace('Loginscreen');
    } catch (error) {
      console.error('Error logging out:', error);
    }
    setShowLogoutModal(false);
  };

  const cancelLogout = () => {
    Animated.timing(fadeAnim, {
      toValue: 0,
      duration: 50,
      useNativeDriver: true,
      easing: Easing.linear,
    }).start();

    Animated.timing(scaleAnim, {
      toValue: 0,
      duration: 50,
      useNativeDriver: true,
      easing: Easing.linear,
    }).start(() => {
      setShowLogoutModal(false);
    });
  };

  const fetchKucingImage = async () => {
    try {
      if (databaseToken) {
        let selectedDatabase;
        if (databaseToken === 'TW01') {
          selectedDatabase = 'IFTPetFeeder1';
        } else if (databaseToken === 'PO20') {
          selectedDatabase = 'IFTPetFeeder2';
        }
        const imageCapturePath = `/${selectedDatabase}/IFTPetFeeder/ImageCapture`;
        const snapshot = await firebase.database().ref(imageCapturePath).once('value');
        const data = snapshot.val();
        const imageUrl = data && data.Link ? data.Link : '';
        setKucingImageURL(imageUrl);
      }
    } catch (error) {
      console.error('Error fetching kucing image:', error);
    } finally {
      setRefreshing(false); // Set refreshing ke false setelah gambar selesai dimuat atau terjadi kesalahan
    }
  };  
  
  useEffect(() => {
    if (databaseToken) {
      fetchKucingImage();
    }
  }, [databaseToken]);
  
  // Fungsi refresh untuk mengambil data terbaru dari database
  const handleRefresh = () => {
    setRefreshing(true);
    fetchKucingImage();
  };  
  
  useEffect(() => {
    if (databaseToken) {
      const fetchStorageData = async () => {
        try {
          let selectedDatabase;
          if (databaseToken === 'TW01') {
            selectedDatabase = 'IFTPetFeeder1';
          } else if (databaseToken === 'PO20') {
            selectedDatabase = 'IFTPetFeeder2';
          }
          const snackPath = `/${selectedDatabase}/IFTPetFeeder/Storage/Snack`;
          const staplePath = `/${selectedDatabase}/IFTPetFeeder/Storage/Staple`;
  
          const updateSnackData = (snapshot) => {
            const snackData = snapshot.val();
            if (snackData !== null) {
              setSnackAmountStorage(snackData);
            } else {
              console.log('Snack data not found or invalid');
            }
          };
  
          const updateStapleData = (snapshot) => {
            const stapleData = snapshot.val();
            if (stapleData !== null) {
              setStapleAmountStorage(stapleData);
            } else {
              console.log('Staple data not found or invalid');
            }
          };
  
          const snackRef = firebase.database().ref(snackPath);
          const stapleRef = firebase.database().ref(staplePath);
  
          snackRef.on('value', updateSnackData);
          stapleRef.on('value', updateStapleData);
  
          return () => {
            snackRef.off('value', updateSnackData);
            stapleRef.off('value', updateStapleData);
          };
        } catch (error) {
          console.error('Error fetching storage data:', error);
        }
      };
      
      fetchStorageData();
    }
  }, [databaseToken]);
  
  useEffect(() => {
    if (databaseToken) {
      const fetchTodayData = async () => {
        try {
          let selectedDatabase;
          if (databaseToken === 'TW01') {
            selectedDatabase = 'IFTPetFeeder1';
          } else if (databaseToken === 'PO20') {
            selectedDatabase = 'IFTPetFeeder2';
          }
          const snackPath = `/${selectedDatabase}/History/PerDay/Snack`;
          const staplePath = `/${selectedDatabase}/History/PerDay/Staple`;
          const gramPath = `/${selectedDatabase}/History/PerDay/Gram`;
          const lastAtePath = `/${selectedDatabase}/History/PerDay/LastAte`;
  
          const updateSnackData = (snapshot) => {
            const snackData = snapshot.val();
            if (snackData !== null) {
              setSnackAmountToday(snackData);
            } else {
              console.log('Snack data not found or invalid');
            }
          };
  
          const updateStapleData = (snapshot) => {
            const stapleData = snapshot.val();
            if (stapleData !== null) {
              setStapleAmountToday(stapleData);
            } else {
              console.log('Staple data not found or invalid');
            }
          };
  
          const updateGramData = (snapshot) => {
            const gramData = snapshot.val();
            if (gramData !== null) {
              setGramAmountToday(gramData);
            } else {
              console.log('Gram data not found or invalid');
            }
          };
  
          const updateLastAteData = (snapshot) => {
            const lastAteData = snapshot.val();
            if (lastAteData !== null) {
              setLastAte(lastAteData);
            } else {
              console.log('Last ate data not found or invalid');
            }
          };
  
          const snackRef = firebase.database().ref(snackPath);
          const stapleRef = firebase.database().ref(staplePath);
          const gramRef = firebase.database().ref(gramPath);
          const lastAteRef = firebase.database().ref(lastAtePath);
  
          snackRef.on('value', updateSnackData);
          stapleRef.on('value', updateStapleData);
          gramRef.on('value', updateGramData);
          lastAteRef.on('value', updateLastAteData);
  
          return () => {
            snackRef.off('value', updateSnackData);
            stapleRef.off('value', updateStapleData);
            gramRef.off('value', updateGramData);
            lastAteRef.off('value', updateLastAteData);
          };
        } catch (error) {
          console.error('Error fetching today data:', error);
        }
      };
  
      fetchTodayData();
    }
  }, [databaseToken]);

  const toggleSwitch = async () => {
    setIsEnabled(prev => !prev);
    const newPosition = !isEnabled ? 'Snack' : 'Staple';
    let selectedDatabase;
    if (databaseToken === 'TW01') {
      selectedDatabase = 'IFTPetFeeder1';
    } else if (databaseToken === 'PO20') {
      selectedDatabase = 'IFTPetFeeder2';
    }
    const path = `/${selectedDatabase}/IFTPetFeeder`;
  
    try {
      await firebase.database().ref(path).update({ FoodPosition: newPosition });
      
      firebase.database().ref(path).on('value', (snapshot) => {
        const data = snapshot.val();
        setIsEnabled(data.FoodPosition === 'Snack');
      });
    } catch (error) {
      console.error('Error toggling switch:', error);
    }
  };    

  return (
    <View style={{ backgroundColor: 'white', flex: 1 }}>
    <View style={styles.container}>
      <TouchableOpacity onPress={handleLogout}>
        <View style={styles.circle}>
          <Image source={Fotoprofil} style={{ width: '100%', height: '100%' }} />
        </View>
      </TouchableOpacity>
      <Text style={styles.title}>Hi PetFeedies!</Text>
    </View>
    <Modal
      animationType="none"
      transparent={true}
      visible={showLogoutModal}
      onRequestClose={() => setShowLogoutModal(false)}
    >
      <Animated.View style={[styles.modalContainer, { opacity: fadeAnim }]}>
        <View style={styles.modalContent}>
          <Text style={styles.modalTitle}>Logout</Text>
          <Text style={styles.modalMessage}>Are you sure you want to logout?</Text>
          <View style={styles.modalButtons}>
            <TouchableOpacity
              onPress={confirmLogout}
              style={[styles.confirmButton, { transform: [{ scale: scaleAnim }] }]}
            >
               <Text style={[styles.buttonText, { color: 'white' }]}>Logout</Text>
            </TouchableOpacity>
            <TouchableOpacity
              onPress={cancelLogout}
              style={[styles.cancelButton, { transform: [{ scale: scaleAnim }] }]}
            >
              <Text style={[styles.buttonText, { color: '#0B1C3F' }]}>Cancel</Text>
            </TouchableOpacity>
          </View>
        </View>
      </Animated.View>
    </Modal>
    <ScrollView
      contentContainerStyle={{ flexGrow: 1 }}
      refreshControl={<RefreshControl refreshing={refreshing} onRefresh={handleRefresh} />}
    >
        <View style={styles.box}>
          {kucingImageURL ? (
            <Image source={{ uri: kucingImageURL }} style={{ width: '100%', height: '100%',resizeMode: 'contain' }} />
          ) : null}
        </View>
        <View style={styles.switchLabelContainer}>
          <Text style={styles.switchLabel}>Food Eaten Today</Text>
        </View>
        <View style={styles.imagesContainer}>
        <View style={styles.leftContainer}>
        <Image source={tutup_atas} style={styles.imagetutupatas}/>
          <View style={styles.tabungContainer}>
            <View style={[styles.tabung, { height: `${stapleAmountStorage}%`, backgroundColor: '#E63232' }]}>
              <Text style={[styles.percent, { color: 'black' }]}>{stapleAmountStorage}%</Text>
            </View>
            <Image source={tutup_bawah} style={styles.imagetutupbawah}/>
          </View>
          <Text style={styles.snackLabel}>Staple</Text>
        </View>
        <View style={styles.middleContentContainer}>
          <Text style={styles.switchLabell}>Position Food</Text>
          <Switch
            trackColor={{ false: '#E63232', true: '#159D64' }}
            thumbColor={isEnabled ? '#f4f3f4' : '#f4f3f4'}
            ios_backgroundColor="#3e3e3e"
            onValueChange={toggleSwitch}
            value={isEnabled}
            style={{ transform: [{ scaleX: 2 }, { scaleY: 2 }] }}
          />
          <Text style={styles.switchLabelll}>Last Ate: {'\n'}{lastAte}</Text>
        </View>
        <View style={styles.rightContainer}>
        <Image source={tutup_atas} style={styles.imagetutupatas}/>
          <View style={styles.tabungContainer}>
            <View style={[styles.tabung, { height: `${snackAmountStorage}%`, backgroundColor: '#159D64' }]}>
              <Text style={[styles.percent, { color: 'black' }]}>{snackAmountStorage}%</Text>
            </View>
            <Image source={tutup_bawah} style={styles.imagetutupbawah}/>
          </View>
          <Text style={styles.snackLabel}>Snack</Text>
        </View>
      </View>
        <View style={styles.todayContainer}>
          <Text style={styles.text1}>TODAY</Text>
          <View style={styles.dataContainer}>
            <Text style={styles.amount}>{stapleAmountToday}</Text>
            <Text style={styles.feed}>Staple Feed</Text>
          </View>
          <View style={styles.dataContainer}>
            <Text style={styles.amount}>{snackAmountToday}</Text>
            <Text style={styles.feed}>Snack Feed</Text>
          </View>
          <View style={styles.dataContainer}>
            <Text style={styles.amount}>{gramAmountToday}</Text>
            <Text style={styles.feed}>Gram</Text>
          </View>
        </View>
      </ScrollView>
    </View>
  );  
};

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    marginLeft: 10,
    marginTop: 10,
    backgroundColor: '#FFFFFF',
  },
  circle: {
    width: 50,
    justifyContent: 'center',
    height: 50,
    borderRadius: 30,
    backgroundColor: '#ccc',
    marginRight: 10,
    marginLeft: 10,
    marginTop: 20,
    marginBottom: 10,
  },
  title: {
    color: '#0B1C3F',
    fontSize: 21,
    textAlign: 'left',
    width: 261,
    height: 25,
    marginLeft: 10,
    marginRight: 70,
    justifyContent: 'space-between',
  },
  box: {
    width: '100%',
    height: 180,
    backgroundColor: '#ccc',
    marginTop: 15,
    justifyContent: 'flex-end',
    alignItems: 'center',
    borderTopWidth: 1,
    borderBottomWidth: 1,
    borderTopColor: '#787B83',
    borderBottomColor: '#787B83',
  },
  switchLabelContainer: {
    alignItems: 'center',
    marginTop: 30,
  },
  switchLabel: {
    color: '#898787',
    fontSize: 19,
    fontWeight: 'bold',
    justifyContent: 'space-between',
    top:-5,
  },
  switchLabell: {
    marginTop: 60,
    marginBottom: 20,
    color: '#4D3F3F',
    fontSize: 15,
    fontWeight: 'bold',
    borderRadius: 5,
    textAlign: 'center',
    marginLeft: 1,
  },
  switchLabelll: {
    marginTop: 10,
    fontSize: 14,
    fontWeight: 'bold',
    color: '#4D3F3F',
    textAlign: 'center',
    marginLeft: 8,
    textAlign: 'center',
  },
  leftContainer: {
    alignItems: 'center',
    justifyContent: 'space-between',
    marginTop: 40,
    marginBottom : 10
  },
  rightContainer: {
    alignItems: 'center',
    justifyContent: 'space-between',
    marginTop: 40,
    marginBottom : 10
  },
  imagesContainer: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'flex-start',
    paddingHorizontal: 20,
    marginTop: 20,
  },
  middleContentContainer: {
    alignItems: 'center',
    justifyContent: 'center',
  },
  tabungContainer: {
    position: 'relative',
    width: 100,
    height: 182,
    backgroundColor: '#FFFFFF',
    justifyContent: 'flex-end',
    alignItems: 'center',
    borderLeftColor: '#8f7d6f',
    borderLeftWidth : 2,
    borderRightColor: '#8f7d6f',
    borderRightWidth : 2,
    marginTop : 10,
    marginBottom: 0,
  },
  tabung: {
    position: 'absolute',
    bottom: 0,
    left: 0,
    right: 0,
  },
  percent: {
    position: 'absolute',
    bottom: '50%',
    left: '40%',
    transform: [{ translateX: -3 }, { translateY: -10 }],
    fontSize: 16,
    fontWeight: 'bold',
    color: 'black',
  },
  label: {
    fontSize: 17,
    fontWeight: 'bold',
    textAlign: 'center',
  },
  line1: {
    backgroundColor: '#787B83',
    marginTop: 10,
    height: 1,
    width: '1000%',
    borderTopWidth: 1,
    borderColor: '#787B83',
  },
  text1: {
    color: '#0B1C3F',
    fontSize: 15,
    fontWeight: 'bold',
    marginLeft: 7,
    marginTop: 20,
  },
  dataContainer: {
    alignItems: 'center',
  },
  amount: {
    fontSize: 17,
    fontWeight: 'bold',
    color: '#0B1C3F',
  },
  feed: {
    fontSize: 15,
    color: '#0B1C3F',
  },
  line2: {
    backgroundColor: '#787B83',
    marginTop: 5,
    borderTopWidth: 1,
    borderColor: '#787B83',
    height: 1,
    width: '1000%',
  },
  snackLabel: {
    fontSize: 18,
    textAlign: 'center',
    color: '#0B1C3F',
    position: 'absolute',
    top: -40,
    left: '65%',
    transform: [{ translateX: -40 }],
  },
  todayContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginTop: 35,
    paddingHorizontal: 20,
    right:5,
  },
  modalContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: 'rgba(0, 0, 0, 0.5)',
  },
  modalContent: {
    backgroundColor: 'white',
    padding: 20,
    borderRadius: 10,
    alignItems: 'center',
    elevation: 5,
  },
  modalTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    marginBottom: 10,
  },
  modalMessage: {
    fontSize: 16,
    marginBottom: 20,
  },
  modalButtons: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    width: '100%',
  },
  confirmButton: {
    backgroundColor: '#4682B4',
    paddingVertical: 10,
    paddingHorizontal: 20,
    borderRadius: 5,
  },
  cancelButton: {
    backgroundColor: '#A9A9A9',
    paddingVertical: 10,
    paddingHorizontal: 20,
    borderRadius: 5,
  },
  buttonText: {
    fontWeight: 'bold',
    fontSize: 16,
  },
  imagetutupatas : {
    width : 115,
    height : 20,
    position : 'absolute',
    bottom : '92%'
  },
  imagetutupbawah :{
    width : 110,
    height : 15.5,
    position : 'absolute',
    top : '98%'
  }
});

export default Header;