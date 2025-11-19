import { BrowserRouter } from "react-router-dom";
import "./App.css";
import Layout from "./layout";

function App() {
  return (
    <>
      <BrowserRouter>
        <Layout>
          <div className="flex gap-8 w-full">
            <div className="flex-1">
              <h1 className="flex-1text-2xl font-bold bg-blue-100 p-8 rounded-2xl">
                Welcome to the App
              </h1>
            </div>
            <div className="flex-1">
              <h1 className="text-2xl font-bold bg-blue-300 p-8 rounded-2xl">
                This is the Application
              </h1>
            </div>
          </div>
          <p>This is the main content area.</p>
        </Layout>
      </BrowserRouter>
    </>
  );
}

export default App;
