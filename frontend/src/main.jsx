// import { StrictMode } from "react";
// import { createRoot } from "react-dom/client";
// import "./index.css";
// import App from "./App.jsx";
// // import { Header , Footer } from './components/index.js'

// createRoot(document.getElementById("root")).render(
//   <StrictMode>
//     <App />
//   </StrictMode>
// );

import "./index.css";
import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import "./index.css";
import { BrowserRouter } from "react-router-dom";
import Routing from "./routes.jsx";
import { Header, Footer } from "./components";
// import "@fontsource/inter";

createRoot(document.getElementById("root")).render(
  <StrictMode>
    <BrowserRouter>
      {/* <Header /> */}
      <Routing />
      {/* <Footer /> */}
    </BrowserRouter>
  </StrictMode>
);
