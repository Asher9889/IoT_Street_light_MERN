// import { Routes, Route } from "react-router-dom";
// import Main from "./components/Main/Main";
// import NotFound from "./components/NotFound";
// import LoginForm from "./components/Login/LoginForm";

// const routesData = [
//   {
//     path: "/",
//     element: <Main />,
//   },
//   {
//     path: "/register",
//     element: <LoginForm />,
//   },
// ];

// function Routing() {
//   return (
//     <Routes>
//       {routesData.map((route, index) => (
//         <Route key={index} path={route.path} element={route.element} />
//       ))}
//       <Route path="*" element={<NotFound />} />
//     </Routes>
//   );
// }

// export default Routing;

import { Routes, Route, useLocation } from "react-router-dom";
import { Header, Footer } from "./components";
import Main from "./components/Main/Main";
import LoginForm from "./components/Login/LoginForm";
import NotFound from "./components/NotFound";

function Routing() {
  const location = useLocation();

  // Define paths where header/footer should be hidden
  const hideLayoutPaths = ["/register"];
  const hideLayout = hideLayoutPaths.includes(location.pathname);

  return (
    <>
      {!hideLayout && <Header />}

      <Routes>
        <Route path="/" element={<Main />} />
        <Route path="/register" element={<LoginForm />} />
        <Route path="*" element={<NotFound />} />
      </Routes>

      {!hideLayout && <Footer />}
    </>
  );
}

export default Routing;
