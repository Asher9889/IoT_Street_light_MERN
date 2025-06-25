import express from "express";
import cors from "cors";
import apiRoutes from "./routes"
const app = express();

app.use(cors());
app.use(express.json({limit: "10mb"}));
app.use(express.urlencoded({extended: true}))


app.use("/api", apiRoutes)


app.listen(5000, ()=>{
    console.log(`App started using port 5000`)
})