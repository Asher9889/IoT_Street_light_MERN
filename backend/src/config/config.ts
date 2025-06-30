import dotenv from "dotenv"

dotenv.config();

type Config = {
    port: number;
    mongoDBURL: string;
    dbName: string;
}


const config: Config = {
    port: Number(process.env.PORT),
    mongoDBURL : String(process.env.MONGODB_URL),
    dbName: String(process.env.MONGO_DB_NAME),
}

export default config;