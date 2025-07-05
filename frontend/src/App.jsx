// import { useState } from "react";
// import "./index.css";
// // import Header from "./components/Header/Header";
// import Header from "./components/Header/Header";
// import Main from "./components/Main";
// import Footer from "./components/Footer";
// function App() {
//   const [count, setCount] = useState(0);

//   return (
//     <>
//       <div className="font-sans h-screen">
//         <Header />
//         <Main />
//         <Footer />
//       </div>
//     </>
//   );
// }

// export default App;

import { useState } from "react";
import "./index.css";
// // import Header from "./components/Header/Header";
// import Header from "./components/Header/Header";
import Main from "./components/Main";
// import Footer from "./components/Footer";
function App() {
  const [count, setCount] = useState(0);

  return (
    <>
      <Main />
    </>
  );
}

export default App;
