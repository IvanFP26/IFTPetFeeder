import { StyleSheet, Text, View, TouchableOpacity, Image } from 'react-native';
import React from 'react';
import { HomeON, HomeOFF, SchedulingON, SchedulingOFF, FeedON, FeedOFF, HistoryON, HistoryOFF } from '../../../assets';

const Icon = ({ label, focus, page }) => {
  let iconSource;
  let labelText;
  let labelStyle;

  if (label === 'Home') {
    iconSource = focus ? HomeON : HomeOFF;
    labelText = 'Home';
  } else if (label === 'Scheduling') {
    iconSource = focus ? SchedulingON : SchedulingOFF;
    labelText = 'Scheduling';
  } else if (label === 'Feed') {
    iconSource = focus ? FeedON : FeedOFF;
    labelText = 'Feed';
  } else if (label === 'History') {
    iconSource = focus ? HistoryON : HistoryOFF;
    labelText = 'History';
  } else {
    iconSource = SchedulingON;
    labelText = 'Unknown';
  }

  labelStyle = focus ? styles.labelFocused : styles.label;

  return (
    <View style={styles.iconContainer}>
      <Image source={iconSource} style={styles.icon} resizeMode="contain" />
      <Text style={labelStyle}>{labelText}</Text>
    </View>
  );
};

const BottomNavigator = ({ state, descriptors, navigation }) => {
  return (
    <View style={styles.container}>
      {state.routes.map((route, index) => {
        const { options } = descriptors[route.key];
        const label =
          options.tabBarLabel !== undefined
            ? options.tabBarLabel
            : options.title !== undefined
            ? options.title
            : route.name;

        const isFocused = state.index === index;

        const onPress = () => {
          const event = navigation.emit({
            type: 'tabPress',
            target: route.key,
            canPreventDefault: true,
          });

          if (!isFocused && !event.defaultPrevented) {
            navigation.navigate(route.name, route.params);
          }
        };

        const onLongPress = () => {
          navigation.emit({
            type: 'tabLongPress',
            target: route.key,
          });
        };

        return (
          <TouchableOpacity
            key={index}
            accessibilityRole="button"
            accessibilityState={isFocused ? { selected: true } : {}}
            accessibilityLabel={options.tabBarAccessibilityLabel}
            testID={options.tabBarTestID}
            onPress={onPress}
            onLongPress={onLongPress}
          >
            <Icon label={label} focus={isFocused} />
          </TouchableOpacity>
        );
      })}
    </View>
  );
};

export default BottomNavigator;

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    backgroundColor: 'white',
    paddingTop: 15,
    paddingBottom: 15,
    paddingHorizontal: 35,
    justifyContent: 'space-between',
    marginHorizontal: -15,
  },
  iconContainer: {
    alignItems: 'center',
  },
  icon: {
    width: 34, // Lebar yang diinginkan
    height: 25, // Tinggi yang diinginkan
  },
  label: {
    marginTop: 5,
    fontWeight: 'normal',
    color: '#4B4B4B',
  },
  labelFocused: {
    marginTop: 5,
    fontWeight: 'bold',
    color: '#FF521B',
  },
});