const AppContext = {};

function processArray(array) {
  let result = "[";
  for (let i = 0; i < array.length; i++) {
    result += array[i];
    if (i < array.length - 1) {
      result += ", ";
    }
  }
  result += "]";
  return result;
}

export { processArray, AppContext };
