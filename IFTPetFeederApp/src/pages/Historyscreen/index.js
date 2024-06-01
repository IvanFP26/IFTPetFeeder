import React, { useState, useEffect } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, Alert, Image, RefreshControl } from 'react-native';
import { Icon } from 'react-native-elements';
import firebase from '@react-native-firebase/app';
import '@react-native-firebase/database';
import AsyncStorage from '@react-native-async-storage/async-storage';

// Import icon trash
const trashIcon = require('../../assets/icon/trash.png');

const HistoryScreen = ({ navigation }) => {
    const [selectedDatabase, setSelectedDatabase] = useState('');
    const [historyData, setHistoryData] = useState([]);
    const [refreshing, setRefreshing] = useState(false); // State untuk mengontrol status refresh

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

    useEffect(() => {
        if (selectedDatabase) {
            const historyRef = firebase.database().ref(`/${selectedDatabase}/History/AllTime`);

            const fetchHistoryData = async () => {
                try {
                    const snapshot = await historyRef.once('value');
                    const data = snapshot.val();
                    if (data) {
                        const dataArray = Object.values(data);
                        // Sort the data by date and time
                        dataArray.sort((a, b) => {
                            const dateA = new Date(a.Time + ' ' + a.Hour);
                            const dateB = new Date(b.Time + ' ' + b.Hour);
                            return dateB - dateA;
                        });
                        setHistoryData(dataArray);
                    } else {
                        setHistoryData([]); // Set historyData menjadi array kosong jika tidak ada data
                    }
                } catch (error) {
                    console.error("Error fetching history data:", error);
                }
            };

            fetchHistoryData();

            // Listener for any changes in the history data
            const onHistoryDataChanged = (snapshot) => {
                const newDataItem = snapshot.val();
                setHistoryData(prevData => {
                    const updatedData = [...prevData];
                    const index = updatedData.findIndex(item => item.Time === newDataItem.Time);
                    if (index !== -1) {
                        updatedData[index] = newDataItem;
                    } else {
                        updatedData.push(newDataItem);
                    }
                    // Sort the updated data by date and time
                    updatedData.sort((a, b) => {
                        const dateA = new Date(a.Time + ' ' + a.Hour);
                        const dateB = new Date(b.Time + ' ' + b.Hour);
                        return dateB - dateA;
                    });
                    return updatedData;
                });
            };

            historyRef.on('child_changed', onHistoryDataChanged);

            return () => {
                historyRef.off('child_changed', onHistoryDataChanged);
            };
        }
    }, [selectedDatabase]);  

    const handleDeleteAllHistory = async () => {
        Alert.alert(
            "Konfirmasi",
            "Yakin ingin menghapus semua riwayat?",
            [
                {
                    text: "Batal",
                    style: "cancel"
                },
                {
                    text: "Ya",
                    onPress: async () => {
                        try {
                            await firebase.database().ref(`/${selectedDatabase}/History/AllTime`).remove();
                            setHistoryData([]);
                        } catch (error) {
                            console.error("Error deleting all history:", error);
                        }
                    }
                }
            ]
        );
    };

    // Metode untuk menangani refresh
    const handleRefresh = async () => {
        setRefreshing(true); // Mengatur status refresh menjadi true
        try {
            // Memuat ulang data riwayat
            const snapshot = await firebase.database().ref(`/${selectedDatabase}/History/AllTime`).once('value');
            const data = snapshot.val();
            if (data) {
                const dataArray = Object.values(data);
                // Sort the data by date and time
                dataArray.sort((a, b) => {
                    const dateA = new Date(a.Time + ' ' + a.Hour);
                    const dateB = new Date(b.Time + ' ' + b.Hour);
                    return dateB - dateA;
                });
                setHistoryData(dataArray);
            } else {
                setHistoryData([]); // Set historyData menjadi array kosong jika tidak ada data
            }
        } catch (error) {
            console.error("Error refreshing history data:", error);
        } finally {
            setRefreshing(false); // Mengatur status refresh menjadi false setelah selesai
        }
    };

    return (
        <View style={styles.container}>
            <View style={styles.header}>
                <View style={styles.headerLeft}>
                    <TouchableOpacity style={styles.backButton} onPress={() => navigation.navigate('Home')}>
                        <Icon name="arrow-back" type="material" size={30} color="black" />
                    </TouchableOpacity>
                    <Text style={styles.title}>History</Text>
                </View>
                <TouchableOpacity onPress={handleDeleteAllHistory} style={styles.deleteButton}>
                    <Image source={trashIcon} style={styles.trashIcon} />
                </TouchableOpacity>
            </View>

            {/* ScrollView dengan properti refreshControl */}
            <ScrollView 
                style={styles.scrollView}
                refreshControl={
                    <RefreshControl 
                        refreshing={refreshing} // Status refresh
                        onRefresh={handleRefresh} // Metode untuk menangani refresh
                    />
                }
            >
                {historyData.map((item, index) => (
                    <View key={index} style={styles.itemContainer}>
                        <View style={styles.itemInfoContainer}>
                            <Text style={styles.itemText}>{item.Hour}</Text>
                            <Text style={styles.itemText}>{item.Time}</Text>
                        </View>
                        <View style={styles.foodInfoContainer}>
                            <Text style={styles.itemText}>{item['Food Position']}</Text>
                            <Text style={styles.itemText}>{item.Gram} gram</Text>
                        </View>
                    </View>
                ))}
            </ScrollView>
        </View>
    );
};

const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: 'white',
    },
    header: {
        flexDirection: 'row',
        justifyContent: 'space-between',
        alignItems: 'center',
        paddingHorizontal: 20,
        marginTop: 20,
        marginBottom: 10,
    },
    headerLeft: {
        flexDirection: 'row',
        alignItems: 'center',
    },
    title: {
        color: '#0B1C3F',
        fontSize: 22,
        fontFamily: 'Roboto',
        marginLeft: 100,
        textAlign: 'center',
    },
    deleteButton: {
        padding: 7,
        borderRadius: 5,
    },
    trashIcon: {
        width: 24,
        height: 24,
        tintColor: 'black',
    },
    scrollView: {
        flex: 1,
        marginTop: 8,
        paddingHorizontal: 11,
    },
    itemContainer: {
        flexDirection: 'row',
        justifyContent: 'space-between',
        marginTop: 18,
        paddingHorizontal: 10,
        borderColor: '#ccc',
        borderRadius: 8,
        paddingVertical: 10,
        borderWidth:2,
        borderColor:'#DEA369'
    },
    itemInfoContainer: {
        flex: 1,
        alignItems: 'flex-start',
    },
    foodInfoContainer: {
        flex: 1,
        alignItems: 'flex-end',
    },
    itemText: {
        color: '#0B1C3F',
        fontSize: 18,
    },
    backButton: {
        zIndex: 999,
    },
});

export default HistoryScreen;
