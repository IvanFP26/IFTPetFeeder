import React, { useState } from 'react';

const MyComponent = () => {
  const [spinnerValue, setSpinnerValue] = useState(0);

  const handleSpinnerChange = (newValue) => {
    // Handler untuk event spinnerStateChange
    setSpinnerValue(newValue);
  };

  return (
    <div>
      <span>Spinner Value: {spinnerValue}</span>
      <button onClick={() => handleSpinnerChange(spinnerValue + 1)}>Increase</button>
      <button onClick={() => handleSpinnerChange(spinnerValue - 1)}>Decrease</button>
    </div>
  );
};

export default MyComponent;