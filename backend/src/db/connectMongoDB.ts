import mongoose from "mongoose";
import { config } from "../config";

async function connectMongoDB():Promise<void> {
    try {
        const connection = await mongoose.connect(config.mongoDBURL + config.dbName)
        console.log(`Mongoose connected to ${connection.connection.name} DB`)
    } catch (error:any) {
        console.log(`Falied to connect to DB ${error.name}`)
    }
} 

mongoose.connection.on("connecting", ()=>{
    console.log("Mongoose trying to connect")
})
mongoose.connection.on("connected", ()=>{
    console.log("Mongoose connected")
})
mongoose.connection.on("disconnect", ()=>{
    console.log("Mongoose disconnected")
})

process.on('SIGINT', async () => {
    await mongoose.connection.close();
    process.exit(0);
})

export default connectMongoDB;